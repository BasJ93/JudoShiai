/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2011 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <cairo.h>
#include <cairo-pdf.h>

#include "judoshiai.h"

#define SIZEX 630
#define SIZEY 891
#define W(_w) ((_w)*pd->paper_width)
#define H(_h) ((_h)*pd->paper_height)

#define NAME_H H(0.02)
#define NAME_W W(0.1)
#define NAME_E W(0.05)
#define NAME_S (NAME_H*2.7)
#define CLUB_WIDTH W(0.16)

#define TEXT_OFFSET   W(0.01)
#define TEXT_HEIGHT   (NAME_H*0.6)

#define THIN_LINE     (pd->paper_width < 700.0 ? 1.0 : pd->paper_width/700.0)
#define THICK_LINE    (2*THIN_LINE)
#define MY_FONT "Arial"

extern void get_match_data(GtkTreeModel *model, GtkTreeIter *iter, gint *group, gint *category, gint *number);

#define NUM_PAGES 200
static struct {
    gint cat;
    gint pagenum;
} pages_to_print[NUM_PAGES];

static gint numpages;

static double rownum1, rowheight1;
//static gint cumulative_time;
#define YPOS (1.2*rownum1*rowheight1 + H(0.01))

static gchar *pdf_out = NULL, *template_in = NULL;

typedef enum {
    BARCODE_PS, 
    BARCODE_PDF, 
    BARCODE_PNG
} target_t;

static void paint_surfaces(struct paint_data *pd, 
                           cairo_t *c_pdf, cairo_t *c_png, 
                           cairo_surface_t *cs_pdf, cairo_surface_t *cs_png,
                           gchar *fname)
{
    gdouble w = pd->paper_width;
    gdouble h = pd->paper_height;
    gchar *p = strrchr(fname, '.');
    if (!p)
        return;

    cairo_surface_t *cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 
                                                     2*pd->paper_width, 2*pd->paper_height);
    cairo_t *c = pd->c = cairo_create(cs);

    pd->paper_width *= 2;
    pd->paper_height *= 2;
    paint_category(pd);
    pd->paper_width = w;
    pd->paper_height = h;
    pd->write_results = FALSE;

    pd->c = c_pdf;
    paint_category(pd);
        
    if (pd->page)
        sprintf(p, "-%d.png", pd->page);
    else
        strcpy(p, ".png");

    cairo_scale(c_png, 0.6, 0.6);
    cairo_set_source_surface(c_png, cs, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(c_png), CAIRO_FILTER_BEST);
    cairo_paint(c_png);
    cairo_scale(c_png, 1.0/0.6, 1.0/0.6);

    cairo_show_page(c);
    cairo_show_page(c_pdf);
    cairo_show_page(c_png);
    cairo_surface_write_to_png(cs_png, fname);

    strcpy(p, ".");

    cairo_destroy(c);
    cairo_surface_destroy(cs);
}

void write_png(GtkWidget *menuitem, gpointer userdata)
{
    gint ctg = (gint)userdata, i;
    gchar buf[200];
    cairo_surface_t *cs_pdf, *cs_png;
    cairo_t *c_pdf, *c_png;
    struct judoka *ctgdata = NULL;
    struct compsys sys;

    if (strlen(current_directory) <= 1) {
        return;
    }

    ctgdata = get_data(ctg);
    if (!ctgdata)
        return;

    sys = db_get_system(ctg);

    struct paint_data pd;
    memset(&pd, 0, sizeof(pd));

    //XXXpd.row_height = 1;
    if (print_landscape(ctg)) {
        pd.paper_height = SIZEX;
        pd.paper_width = SIZEY;
        //XXXpd.row_height = 2;
        pd.landscape = TRUE;
    } else {
        pd.paper_height = SIZEY;
        pd.paper_width = SIZEX;
    }
    pd.total_width = 0;
    pd.category = ctg;

    snprintf(buf, sizeof(buf)-10, "%s/%s.pdf", current_directory, txt2hex(ctgdata->last));

    cs_pdf = cairo_pdf_surface_create(buf, pd.paper_width, pd.paper_height);
    cs_png = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 
                                        1.2*pd.paper_width, 1.2*pd.paper_height);

    c_pdf = cairo_create(cs_pdf);
    c_png = cairo_create(cs_png);

    paint_surfaces(&pd, c_pdf, c_png, cs_pdf, cs_png, buf);

    for (i = 1; i < num_pages(sys); i++) { // print other pages
        pd.page = i;
        paint_surfaces(&pd, c_pdf, c_png, cs_pdf, cs_png, buf);
    }

    cairo_destroy(c_pdf);
    cairo_destroy(c_png);

    cairo_surface_destroy(cs_pdf);
    cairo_surface_destroy(cs_png);

    //cairo_surface_flush(cs_pdf);

    free_judoka(ctgdata);
}

static gchar *get_save_as_name(const gchar *dflt, gboolean opendialog)
{
    GtkWidget *dialog;
    gchar *filename;
    static gchar *last_dir = NULL;

    dialog = gtk_file_chooser_dialog_new (_("Save file"),
                                          GTK_WINDOW(main_window),
                                          opendialog ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          opendialog ? GTK_STOCK_OPEN : GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

    if (last_dir) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), last_dir);
    } else if (database_name[0] == 0) {
        if (current_directory[0] != '.')
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_directory);
        else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    } else {
        gchar *dirname = g_path_get_dirname(database_name);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    }

    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), dflt);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        if (last_dir)
            g_free(last_dir);
        last_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (dialog));
    } else
        filename = NULL;

    gtk_widget_destroy (dialog);

    valid_ascii_string(filename);

    return filename;
}

/* Code encoding:
   bits 8-4: 1 = wide bar, 0 = narrow bar
   bits 3-0: 1 = wide space, 0 = narrow space
 */
static gint get_code_39(gchar key)
{
    switch (key) {
    case ' ': return 0x0a8;
    case '$': return 0x00e;
    case '%': return 0x007;
    case '*': return 0x068;
    case '+': return 0x00b;
    case '-': return 0x038;
    case '.': return 0x128;
    case '/': return 0x00d;
    case '0': return 0x064;
    case '1': return 0x114;
    case '2': return 0x094;
    case '3': return 0x184;
    case '4': return 0x054;
    case '5': return 0x144; // 0x164 ?
    case '6': return 0x0c4;
    case '7': return 0x034;
    case '8': return 0x124;
    case '9': return 0x0a4;
    case 'A': return 0x112;
    case 'B': return 0x092;
    case 'C': return 0x182;
    case 'D': return 0x052;
    case 'E': return 0x142;
    case 'F': return 0x0c2;
    case 'G': return 0x032;
    case 'H': return 0x122;
    case 'I': return 0x0a2;
    case 'J': return 0x062;
    case 'K': return 0x111;
    case 'L': return 0x091;
    case 'M': return 0x181;
    case 'N': return 0x051;
    case 'O': return 0x141;
    case 'P': return 0x0c1;
    case 'Q': return 0x031;
    case 'R': return 0x121;
    case 'S': return 0x0a1;
    case 'T': return 0x061;
    case 'U': return 0x118;
    case 'V': return 0x098;
    case 'W': return 0x188;
    case 'X': return 0x058;
    case 'Y': return 0x148;
    case 'Z': return 0x0c8;
    }
    return 0;
}

static void get_code_39_extended(gchar key, gchar *c1, gchar *c2)
{
    if (key == 0) {*c1 = '%'; *c2 = 'U'; }
    else if (key >= 1 && key <= 26) { *c1 = '$'; *c2 = key - 1 + 'A'; }
    else if (key >= 27 && key <= 31) { *c1 = '%'; *c2 = key - 27 + 'A'; }
    else if (key == 32 || (key >= '0' && key <= '9') || key == '-' || key == '.' || 
             (key >= 'A' && key <= 'Z')) { *c1 = 0; *c2 = key; }
    else if ((key >= 33 && key <= 44) || key == 47 || key == 58) { *c1 = '/'; *c2 = key - 33 + 'A'; }
    else if (key >= 59 && key <= 63) { *c1 = '%'; *c2 = key - 59 + 'F'; }
    else if (key == 64) { *c1 = '%'; *c2 = 'V'; }
    else if (key >= 91 && key <= 95) { *c1 = '%'; *c2 = key - 91 + 'K'; }
    else if (key == 96) { *c1 = '%'; *c2 = 'W'; }
    else if (key >= 97 && key <= 122) { *c1 = '+'; *c2 = key - 97 + 'A'; }
    else if (key >= 123 && key <= 127) { *c1 = '%'; *c2 = key - 123 + 'P'; }
    else {
        g_print("Error %s:%d: key=%d\n", __FUNCTION__, __LINE__, key);
        *c1 = 0;
        *c2 = 0;
    }
}

static void draw_code_39_pattern(char key, struct paint_data *pd, double bar_height)
{
    int          iterator;
    double       x, y;
    int          bars = TRUE;
    int          code39 = get_code_39(key);

    /* set color */
    cairo_set_operator (pd->c, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgb (pd->c, 0, 0, 0);

    /* get current point */
    cairo_get_current_point (pd->c, &x, &y);

    /* written a slim space move 1.0 */
    cairo_move_to (pd->c, x + 1.0, y);

    /* write to the surface */
    iterator = 0;
    while (iterator < 5) {
		
        /* get current point */
        cairo_get_current_point (pd->c, &x, &y);
		
        /* write a bar or an space */
        if (bars) {
            /* write a bar */
            if (code39 & (0x100 >> iterator)) {
                //if (unit->bars[iterator] == '1') {

                /* write a flat bar */
                cairo_set_source_rgb (pd->c, 0, 0, 0); 
                cairo_set_line_width (pd->c, THIN_LINE);
                cairo_line_to (pd->c, x, y + bar_height);

                cairo_move_to (pd->c, x + 1, y);
                cairo_line_to (pd->c, x + 1, y + bar_height);

                cairo_move_to (pd->c, x + 2, y);
                cairo_line_to (pd->c, x + 2, y + bar_height);
				
            }else {
                /* write an slim bar */
                cairo_set_source_rgb (pd->c, 0, 0, 0);
                cairo_set_line_width (pd->c, THIN_LINE);
                cairo_line_to (pd->c, x, y + bar_height);
            }

            /* make the path visible */
            cairo_stroke (pd->c);

            /* write a bar */
            if (code39 & (0x100 >> iterator)) {
                //if (unit->bars[iterator] == '1') {
                /* written a flat line move 2.0 */
                cairo_move_to (pd->c, x + 3.0, y);
            }else {
                /* written a slim line move 1.0 */
                cairo_move_to (pd->c, x + 1.0, y);
            }

        }else {
            /* write a space */
            if (code39 & (0x8 >> iterator)) {
                //if (unit->spaces[iterator] == '1') {
                /* written a flat space move 2.0 */
                cairo_move_to (pd->c, x + 3.0, y);
            }else {
                /* written a slim space move 1.0 */
                cairo_move_to (pd->c, x + 1.0, y);
            }

        } /* end if */

        /* check for last bar */
        if (bars && iterator == 4) {
            break;
        }

        /* update bars to write the opposite */
        bars = ! bars;

        /* update the iterator if written a space */
        if (bars) {
            iterator++;
        } /* end if */
		
    } /* end while */
	
    return;
}

static void draw_code_39_string(gchar *s, struct paint_data *pd, double bar_height, gboolean extended)
{
    if (!s)
        return;

    cairo_save(pd->c);
    draw_code_39_pattern('*', pd, bar_height); 

    for (; *s; s++) {
        if (extended) {
            gchar c1, c2;
            get_code_39_extended(*s, &c1, &c2);
            if (c1)
                draw_code_39_pattern(c1, pd, bar_height);
            draw_code_39_pattern(c2, pd, bar_height);
        } else
            draw_code_39_pattern(*s, pd, bar_height);
    }

    draw_code_39_pattern ('*', pd, bar_height); 
    cairo_restore(pd->c);
}

#define NUM_WN_TEXTS 32
#define IS_PICTURE 1

struct wn_data_s {
    gdouble x, y, width, height, angle, size;
    gdouble red, green, blue;
    gint slant, weight, flags;
    gchar *font, *text;
};

static struct wn_data_s wn_texts_default[] = {
    {22.0, 15.0, 0.0, 0.0, 0.0, 12.0, 0.0, 0.0, 0.0, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD, 0, "Arial", "%REGCATEGORY%"},
    {36.0, 15.0, 0.0, 0.0, 0.0, 12.0, 0.0, 0.0, 0.0, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD, 0, "Arial", "%CLUBCOUNTRY%"},
    {22.0, 23.0, 0.0, 0.0, 0.0, 12.0, 0.0, 0.0, 0.0, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD, 0, "Arial", "%LAST%, %FIRST%"},
    {60.0, 44.0, 0.0, 0.0, 0.0, 12.0, 0.0, 0.0, 0.0, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD, 0, "Arial", "%INDEX%"},
    {55.0, 34.0, 0.0, 0.0, 0.0, 12.0, 0.0, 0.0, 0.0, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD, 0, "Arial", "%BARCODE%"},
    {22.0, 30.0, 0.0, 0.0, 0.0, 12.0, 0.0, 0.0, 0.0, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD, 0, "Arial", "%WEIGHTTEXT%"},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0.0, 0.0, 0.0, 0, NULL, NULL}
};
static struct wn_data_s wn_texts[NUM_WN_TEXTS] = {{0}};
static gint num_wn_texts = 0;
static gdouble note_w, note_h;
static gdouble border;
static gchar *background;

#define X_MM(_a) (_a*pd->paper_width/pd->paper_width_mm)
#define Y_MM(_a) (_a*pd->paper_height/pd->paper_height_mm)
#define IS_STR(_a) (strncmp(&(wn_texts[t].text[k]), _a, (len=strlen(_a)))==0)
#define OUT  do { ok = FALSE; break; } while (0)
#define NEXT_TOKEN  do { p = get_token(p); if (!p) { ok = FALSE; goto out; }} while (0)

static gchar *get_token(gchar *text)
{
    gchar *p = strchr(text, ' ');
    if (!p)
        return NULL;
    while (*p == ' ')
        p++;

    if (*p <= ' ')
        return NULL;

    return p;
}

static void read_print_template(gchar *templatefile, GtkPrintContext *context)
{
    gint t = 0;
    gdouble paper_width_mm = 210.0;
    gdouble paper_height_mm = 297.0;

    if (context) {
        GtkPageSetup *setup = gtk_print_context_get_page_setup(context);

        paper_width_mm = gtk_page_setup_get_paper_width(setup, GTK_UNIT_MM) -
            gtk_page_setup_get_left_margin(setup, GTK_UNIT_MM) -
            gtk_page_setup_get_right_margin(setup, GTK_UNIT_MM);

        paper_height_mm = gtk_page_setup_get_paper_height(setup, GTK_UNIT_MM) -
            gtk_page_setup_get_top_margin(setup, GTK_UNIT_MM) -
            gtk_page_setup_get_bottom_margin(setup, GTK_UNIT_MM);
    }

    note_w = paper_width_mm/2;
    note_h = paper_height_mm/5;

    border = 1.0;
    g_free(background);
    background = NULL;

    // initialize text table
    while (wn_texts_default[t].font) {
        wn_texts[t].x = wn_texts_default[t].x;
        wn_texts[t].y = wn_texts_default[t].y;
        wn_texts[t].width = wn_texts_default[t].width;
        wn_texts[t].height = wn_texts_default[t].height;
        wn_texts[t].angle = wn_texts_default[t].angle;
        wn_texts[t].size = wn_texts_default[t].size;
        wn_texts[t].red = wn_texts_default[t].red;
        wn_texts[t].green = wn_texts_default[t].green;
        wn_texts[t].blue = wn_texts_default[t].blue;
        wn_texts[t].slant = wn_texts_default[t].slant;
        wn_texts[t].weight = wn_texts_default[t].weight;
        wn_texts[t].flags = 0;
        g_free(wn_texts[t].font);
        wn_texts[t].font = strdup(wn_texts_default[t].font);
        g_free(wn_texts[t].text);
        wn_texts[t].text = strdup(wn_texts_default[t].text);
        t++;
    }
    num_wn_texts = t;

    FILE *f = NULL;
    if (templatefile)
        f = fopen(templatefile, "r");
    if (f) {
        gint slant = CAIRO_FONT_SLANT_NORMAL;
        gint weight = CAIRO_FONT_WEIGHT_NORMAL;
        gdouble x = 0.0, y = 0.0, a = 0.0, size = 12.0;
        gdouble r = 0.0, g = 0.0, b = 0.0, pw = 0.0, ph = 0.0;
        gchar line[128], *font = strdup("Arial");
        gboolean ok = TRUE;
        gint linenum = 0;

        num_wn_texts = 0;

        while (ok && fgets(line, sizeof(line), f)) {
            linenum++;
            gchar *p, *p1 = strchr(line, '\r');
            if (p1)	*p1 = 0;
            p1 = strchr(line, '\n');
            if (p1)	*p1 = 0;

            p1 = line;
            while (*p1 == ' ')
                p1++;

            if (*p1 == 0)
                continue;

            if (p1[0] == '#') { // comment
                continue;
            } else if (strncmp(p1, "text ", 5) == 0 ||
                       strncmp(p1, "picture ", 8) == 0) {
                gboolean pic = strncmp(p1, "picture ", 8) == 0;
                p = p1;
                NEXT_TOKEN;
                x = atof(p);
                NEXT_TOKEN;
                y = atof(p);
                if (pic) { // picture size
                    NEXT_TOKEN;
                    pw = atof(p);
                    NEXT_TOKEN;
                    ph = atof(p);
                }
                NEXT_TOKEN;
                a = -atof(p)*2.0*3.14159265/360.0;
                NEXT_TOKEN;
                wn_texts[num_wn_texts].x = x;
                wn_texts[num_wn_texts].y = y;
                if (pic) {
                    wn_texts[num_wn_texts].width = pw;
                    wn_texts[num_wn_texts].height = ph;
                    wn_texts[num_wn_texts].flags = IS_PICTURE;
                } else {
                    wn_texts[num_wn_texts].flags = 0;
                }
                wn_texts[num_wn_texts].angle = a;
                wn_texts[num_wn_texts].size = size;
                wn_texts[num_wn_texts].red = r;
                wn_texts[num_wn_texts].green = g;
                wn_texts[num_wn_texts].blue = b;
                wn_texts[num_wn_texts].slant = slant;
                wn_texts[num_wn_texts].weight = weight;
                g_free(wn_texts[num_wn_texts].font);
                wn_texts[num_wn_texts].font = strdup(font);
                g_free(wn_texts[num_wn_texts].text);
                wn_texts[num_wn_texts].text = strdup(p);
                if (num_wn_texts < NUM_WN_TEXTS - 1) 
                    num_wn_texts++;
            } else if (strncmp(p1, "font ", 5) == 0) {
                p = p1;
                NEXT_TOKEN;
                g_free(font);
                font = strdup(p);
            } else if (strncmp(p1, "fontsize ", 9) == 0) {
                p = p1;
                NEXT_TOKEN;
                size = atof(p);
            } else if (strncmp(p1, "fontslant ", 10) == 0) {
                if (strstr(p1 + 10, "italic"))
                    slant = CAIRO_FONT_SLANT_ITALIC;
                else
                    slant = CAIRO_FONT_SLANT_NORMAL;
            } else if (strncmp(p1, "fontweight ", 11) == 0) {
                if (strstr(p1 + 11, "bold"))
                    weight = CAIRO_FONT_WEIGHT_BOLD;
                else
                    weight = CAIRO_FONT_WEIGHT_NORMAL;
            } else if (strncmp(p1, "background", 10) == 0) {
                p = p1;
                NEXT_TOKEN;
                g_free(background);
                background = strdup(p);
            } else if (strncmp(p1, "notesize ", 9) == 0) {
                p = p1;
                NEXT_TOKEN;
                note_w = atof(p);
                NEXT_TOKEN;
                note_h = atof(p);
            } else if (strncmp(p1, "cardgeom ", 9) == 0) {
                p = p1;
                NEXT_TOKEN;
                note_w = paper_width_mm/atof(p);
                NEXT_TOKEN;
                note_h = paper_height_mm/atof(p);
            } else if (strncmp(p1, "color ", 6) == 0) {
                p = p1;
                NEXT_TOKEN;
                r = atof(p);
                NEXT_TOKEN;
                g = atof(p);
                NEXT_TOKEN;
                b = atof(p);
            } else if (strncmp(p1, "border ", 7) == 0) {
                p = p1;
                NEXT_TOKEN;
                border = atof(p);
            } else { // error
                ok = FALSE;
                break;
            }
        } // while
        g_free(font);
        fclose(f);
 out:
        if (!ok) {
            SHOW_MESSAGE("%s %d: %s", _("Line"), linenum, _("Syntax error"));
            return;
        }
    }
}

static gint find_print_judokas(gchar *where_string)
{
    gint row, numrows;
    gchar request[256];

    num_selected_judokas = 0;

    if (club_text == CLUB_TEXT_CLUB)
        snprintf(request, sizeof(request), 
                 "select \"index\" from competitors "
                 "where \"deleted\"&1=0 %s "
                 "order by \"club\",\"last\",\"first\" asc",
                 where_string ? where_string : "");
    else if (club_text == CLUB_TEXT_COUNTRY)
        snprintf(request, sizeof(request), 
                 "select \"index\" from competitors "
                 "where \"deleted\"&1=0 %s "
                 "order by \"country\",\"last\",\"first\" asc",
                 where_string ? where_string : "");
    else
        snprintf(request, sizeof(request), 
                 "select \"index\" from competitors "
                 "where \"deleted\"&1=0 %s "
                 "order by \"country\",\"club\",\"last\",\"first\" asc",
                 where_string ? where_string : "");

    numrows = db_get_table(request);
    if (numrows < 0)
        return numrows;

    for (row = 0; row < numrows && row < TOTAL_NUM_COMPETITORS; row++) {
        gchar *ix = db_get_data(row, "index");
        selected_judokas[num_selected_judokas++] = atoi(ix);
    }

    db_close_table();

    return num_selected_judokas;
}

static gint get_num_pages(struct paint_data *pd)
{
    gint i, pages = 1;
    gdouble x = 0.0, y = 0.0;

    for (i = 0; i < num_selected_judokas; i++) {
        x += X_MM(note_w);
        if (x + X_MM(note_w) > pd->paper_width + 1.0) {
            x = 0.0;
            y += Y_MM(note_h);
            if (y + Y_MM(note_h) > pd->paper_height + 1.0) {
                y = 0.0;
                pages++;
            }
        }
    }

    if (x == 0.0 && y == 0.0)
        pages--;

    return pages;
}

static gint get_starting_judoka(struct paint_data *pd, gint what, gint pagenum)
{
    gint i, pages = 1;
    gdouble x = 0.0, y = 0.0;

    if (what & PRINT_ONE_PER_PAGE)
        return pagenum-1;

    for (i = 0; i < num_selected_judokas; i++) {
        if (pages == pagenum)
            return i;
        x += X_MM(note_w);
        if (x + X_MM(note_w) > pd->paper_width + 1.0) {
            x = 0.0;
            y += Y_MM(note_h);
            if (y + Y_MM(note_h) > pd->paper_height + 1.0) {
                y = 0.0;
                pages++;
            }
        }
    }
    return num_selected_judokas;
}

static void paint_weight_notes(struct paint_data *pd, gint what, gint page)
{
    gint row, t = 0, current_page = 0;
    gchar buf[100];
    cairo_text_extents_t extents;
    gdouble sx = 1.0, sy = 1.0;
    cairo_surface_t *image = NULL;

    if (num_selected_judokas <= 0)
        return;

    cairo_set_antialias(pd->c, CAIRO_ANTIALIAS_NONE);

    cairo_set_source_rgb(pd->c, 1.0, 1.0, 1.0);
    cairo_rectangle(pd->c, 0.0, 0.0, pd->paper_width, pd->paper_height);
    cairo_fill(pd->c);

    cairo_select_font_face(pd->c, MY_FONT,
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(pd->c, TEXT_HEIGHT);
    cairo_text_extents(pd->c, "Xjgy", &extents);

    cairo_set_line_width(pd->c, THIN_LINE);
    cairo_set_source_rgb(pd->c, 0.0, 0.0, 0.0);

    // background picture
    if (background) {
        gint w, h;
        image = cairo_image_surface_create_from_png(background);
        w = cairo_image_surface_get_width(image);
        h = cairo_image_surface_get_height(image);
        sx = X_MM(note_w)/w;
        sy = Y_MM(note_h)/h;
    }

    //numpages = num_selected_judokas/10 + 1;
        
    gdouble x = 0.0, y = 0.0;
    double bar_height = H(0.02);
    gchar id_str[10];
    gchar request[128];
    gint start = get_starting_judoka(pd, what, page);
    gint stop = get_starting_judoka(pd, what, page+1);

    for (row = start; row < stop; row++) {
        snprintf(request, sizeof(request), 
                 "select * from competitors "
                 "where \"deleted\"&1=0 and \"index\"=%d ",
                 selected_judokas[row]);                 

        gint numrows = db_get_table(request);

        if (numrows < 0)
            continue;
        if (numrows < 1) {
            db_close_table();
            continue;
        }

        gchar *last = db_get_data(0, "last");
        gchar *first = db_get_data(0, "first");
        gchar *club = db_get_data(0, "club");
        gchar *country = db_get_data(0, "country");
        gchar *cat = db_get_data(0, "regcategory");
        gchar *realcat = db_get_data(0, "category");
        gchar *ix = db_get_data(0, "index");
        gchar *id = db_get_data(0, "id");
        gchar *weight = db_get_data(0, "weight");
        gchar *yob = db_get_data(0, "birthyear");
        gchar *grade = db_get_data(0, "belt");

        struct judoka j;
        j.club = club;
        j.country = country;

        if (image && cairo_surface_status(image) == CAIRO_STATUS_SUCCESS) {
            cairo_save(pd->c);
            cairo_scale(pd->c, sx, sy);
            cairo_set_source_surface(pd->c, image, x/sx, y/sy);
            cairo_paint(pd->c);
            cairo_restore(pd->c);
        }

        if (border > 0.0) {
            cairo_set_line_width(pd->c, border);
            cairo_rectangle(pd->c, x, y, X_MM(note_w), Y_MM(note_h));
            cairo_stroke(pd->c);
        }

        sprintf(id_str, "%04d", atoi(ix));

        for (t = 0; t < num_wn_texts; t++) {
            cairo_save(pd->c);
            cairo_set_source_rgb(pd->c, wn_texts[t].red, wn_texts[t].green, wn_texts[t].blue);
            cairo_translate(pd->c, x + X_MM(wn_texts[t].x), y + Y_MM(wn_texts[t].y));
            cairo_move_to(pd->c, 0, 0);
            //cairo_move_to(pd->c, x + X_MM(wn_texts[t].x), y + Y_MM(wn_texts[t].y));
            //if (wn_texts[t].flags & IS_PICTURE)
            cairo_rotate(pd->c, wn_texts[t].angle);
            cairo_select_font_face(pd->c, wn_texts[t].font,
                                   wn_texts[t].slant,
                                   wn_texts[t].weight);
            cairo_set_font_size(pd->c, wn_texts[t].size);
            gint k = 0, d = 0;
            gchar ch;
            while ((ch = wn_texts[t].text[k])) {
                if (ch != '%') {
                    buf[d++] = ch;
                    k++;
                } else {
                    gint len = 1;
                    if (IS_STR("%REGCATEGORY%"))
                        d += sprintf(buf + d, "%s", cat);
                    else if (IS_STR("%REALCATEGORY%"))
                        d += sprintf(buf + d, "%s", realcat);
                    else if (IS_STR("%LAST%"))
                        d += sprintf(buf + d, "%s", last);
                    else if (IS_STR("%FIRST%"))
                        d += sprintf(buf + d, "%s", first);
                    else if (IS_STR("%CLUB%"))
                        d += sprintf(buf + d, "%s", club);
                    else if (IS_STR("%COUNTRY%"))
                        d += sprintf(buf + d, "%s", country);
                    else if (IS_STR("%CLUBCOUNTRY%"))
                        d += sprintf(buf + d, "%s", get_club_text(&j, 0));
                    else if (IS_STR("%INDEX%"))
                        d += sprintf(buf + d, "%s", id_str);
                    else if (IS_STR("%WEIGHT%")) {
                        gint w = atoi(weight);
                        d += sprintf(buf + d, "%d.%d", w/1000, (w%1000)/100);
                    } else if (IS_STR("%YOB%"))
                        d += sprintf(buf + d, "%s", yob);
                    else if (IS_STR("%GRADE%")) {
                        gint belt = atoi(grade);
                        if (belt < 0 || belt > 13)
                            belt = 0;
                        d += sprintf(buf + d, "%s", belts[belt]);
                    } else if (IS_STR("%ID%")) {
                        d += sprintf(buf + d, "%s", id);
                    } else if (IS_STR("%ID-BARCODE%")) {
                        //g_print("id-barcode found, len=%d\n", len);
                        draw_code_39_string(id, pd, bar_height, FALSE);
                    } else if (IS_STR("%ID-BARCODE-EXT%")) {
                        //g_print("id-barcode-ext found, len=%d\n", len);
                        draw_code_39_string(id, pd, bar_height, TRUE);
                    } else if (IS_STR("%BARCODE%")) {
                        //g_print("barcode found, len=%d\n", len);
                        draw_code_39_string(id_str, pd, bar_height, FALSE);
                    } else if (IS_STR("%WEIGHTTEXT%"))
                        d += sprintf(buf + d, "%s", _T(weight));

                    k += len;
                }
            }
            buf[d] = 0;
            if (wn_texts[t].flags & IS_PICTURE) {
                gint w, h;
                cairo_surface_t *img = cairo_image_surface_create_from_png(buf);
                if (img && cairo_surface_status(img) == CAIRO_STATUS_SUCCESS) {
                    w = cairo_image_surface_get_width(img);
                    h = cairo_image_surface_get_height(img);
                    gdouble scx = X_MM(wn_texts[t].width)/w;
                    gdouble scy = Y_MM(wn_texts[t].height)/h;
                    cairo_scale(pd->c, scx, scy);
                    cairo_set_source_surface(pd->c, img, 0, 0);
                    cairo_paint(pd->c);
                    cairo_surface_destroy(img); 
                }                
            } else {
                cairo_show_text(pd->c, buf);
            }
            cairo_restore(pd->c);
        } // for t

        x += X_MM(note_w);
        if (what & PRINT_ONE_PER_PAGE || x + X_MM(note_w) > pd->paper_width + 1.0) {
            x = 0.0;
            y += Y_MM(note_h);
            if (what & PRINT_ONE_PER_PAGE || y + Y_MM(note_h) > pd->paper_height + 1.0) {
                y = 0.0;
                current_page++;
#if 0
                if (current_page == page || page == 0)
                    cairo_show_page(pd->c);
                cairo_set_source_rgb(pd->c, 1.0, 1.0, 1.0);
                cairo_rectangle(pd->c, 0.0, 0.0, pd->paper_width, pd->paper_height);
                cairo_fill(pd->c);
                cairo_set_source_rgb(pd->c, 0.0, 0.0, 0.0);
#endif
            }
        }
        db_close_table();
    } // for (row = 0; row < num_print_judokas; row++)

    //if (x > 0.0 || y > 0.0)
    //cairo_show_page(pd->c);

    if (image)
        cairo_surface_destroy(image);        
}

static gint get_start_time(void)
{
    gint h = 0, m = 0, nh = 0, nm = 0, htxt = 1;
    gchar *p;

    for (p = info_time; *p; p++) {
        if (*p >= '0' && *p <= '9') {
            if (htxt && nh < 2 && h <= 2) {
                h = 10*h + *p - '0';
                nh++;
                if (nh >= 2)
                    htxt = 0;
            } else if (nm < 2 && m <= 5) {
                m = 10*m + *p - '0';
                nm++;
            } else
                break;
        } else if (nh > 0) {
            htxt = 0;
        }
    }

    if (h > 23) h = 0;
    if (m > 59) m = 0;

    return (h*3600 + m*60);
}

static void paint_schedule(struct paint_data *pd)
{
    gint i;
    gchar buf[100];
    cairo_t *c = pd->c;
    cairo_text_extents_t extents;

    cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
    cairo_rectangle(c, 0.0, 0.0, pd->paper_width, pd->paper_height);
    cairo_fill(c);

    cairo_select_font_face(c, MY_FONT,
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(c, TEXT_HEIGHT*1.5);
    cairo_text_extents(c, "Xjgy", &extents);
    rowheight1 = extents.height;

    cairo_set_line_width(c, THIN_LINE);
    cairo_set_source_rgb(c, 0.0, 0.0, 0.0);

    rownum1 = 1;
    cairo_move_to(c, W(0.04), YPOS);
    cairo_show_text(c, _T(schedule));

    rownum1 = 2;
    cairo_move_to(c, W(0.04), YPOS);
    snprintf(buf, sizeof(buf), "%s  %s  %s", 
             info_competition,
             info_date,
             info_place);
    cairo_show_text(c, buf);

#if 0
    for (i = 0; i < NUM_TATAMIS; i++) {
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(match_view[i]));
        cumulative_time = get_start_time();
        category1 = number1 = group1 = -1;
        rownum1 += 2;
        cairo_move_to(c, W(0.04), YPOS);
        sprintf(buf, "MATTO %d", i+1);
        cairo_show_text(c, buf);
        gtk_tree_model_foreach(model, traverse_rows, pd);
    }
#else
    for (i = 1; i <= number_of_tatamis/*NUM_TATAMIS*/; i++) {
        gint old_group = -1, matches_left = 0;
        struct category_data *catdata = category_queue[i].next;
        gint start = get_start_time();
        gint cumulative_time;

        rownum1 += 2;
        cairo_move_to(c, W(0.04), YPOS);
        sprintf(buf, "TATAMI %d", i);
        cairo_show_text(c, buf);

        while (catdata) {
            gint n = 1;

            if (catdata->group != old_group) {
                cumulative_time = 180*matches_left + start;
                rownum1++;
                cairo_move_to(c, W(0.044), YPOS);
                sprintf(buf, "%2d:%02d:  ", cumulative_time/3600, 
                        (cumulative_time%3600)/60);
                cairo_show_text(c, buf);
            }	
            old_group = catdata->group;

            cairo_show_text(c, catdata->category);
            cairo_show_text(c, "  ");

            if (catdata->match_count > 0) {
                n = catdata->match_count - catdata->matched_matches_count;
            } else {
                GtkTreeIter tmp_iter;
                n = 0;
                if (find_iter(&tmp_iter, catdata->index)) {
                    gint k = gtk_tree_model_iter_n_children(current_model, &tmp_iter);
                    n = estim_num_matches[k <= NUM_COMPETITORS ? k : NUM_COMPETITORS];
                }
            }

            if ((catdata->match_status & MATCH_EXISTS) &&
                (catdata->match_status & MATCH_UNMATCHED) == 0 /*n == 0*/)
                goto loop;

            matches_left += n;
        loop:
            catdata = catdata->next;
        }
    }
#endif
}

static gint fill_in_pages(gint category, gint all)
{
    GtkTreeIter iter;
    gboolean ok;
    GtkTreeSelection *selection = 
        gtk_tree_view_get_selection(GTK_TREE_VIEW(current_view));
    gint cat = 0, i;

    numpages = 0;

    ok = gtk_tree_model_get_iter_first(current_model, &iter);
    while (ok) {
        gint index; 
        struct compsys sys;

        gtk_tree_model_get(current_model, &iter,
                           COL_INDEX, &index,
                           -1);
			
        if (all ||
            gtk_tree_selection_iter_is_selected(selection, &iter)) {
            sys = db_get_system(index);
                 
            for (i = 0; i < num_pages(sys) && numpages < NUM_PAGES; i++) {
                pages_to_print[numpages].cat = index;
                pages_to_print[numpages++].pagenum = i;
            }

            if (cat)
                cat = -1;
            else
                cat = index;

            struct category_data *d = avl_get_category(index);
            if (d && all == FALSE)
                d->match_status |= CAT_PRINTED;
        }
        ok = gtk_tree_model_iter_next(current_model, &iter);
    }

    if (numpages == 0) {
        struct compsys sys = db_get_system(category);

        for (i = 0; i < num_pages(sys) && numpages < NUM_PAGES; i++) {
            pages_to_print[numpages].cat = category;
            pages_to_print[numpages++].pagenum = i;
        }
        cat = category;

        struct category_data *d = avl_get_category(category);
        if (d)
            d->match_status |= CAT_PRINTED;
    }
        
    refresh_window();

    return cat;
}

static void begin_print(GtkPrintOperation *operation,
                        GtkPrintContext   *context,
                        gpointer           user_data)
{
    gint what = (gint)user_data & PRINT_ITEM_MASK;
    numpages = 1;

    if (what == PRINT_ALL_CATEGORIES || what == PRINT_SHEET) {
        fill_in_pages((gint)user_data & PRINT_DATA_MASK, 
                      what == PRINT_ALL_CATEGORIES);
        gtk_print_operation_set_n_pages(operation, numpages);
    } else if (what == PRINT_WEIGHING_NOTES) {
        GtkPageSetup *setup = gtk_print_context_get_page_setup(context);
        struct paint_data pd;

	pd.paper_width = gtk_print_context_get_width(context);
	pd.paper_height = gtk_print_context_get_height(context);

        pd.paper_width_mm = gtk_page_setup_get_paper_width(setup, GTK_UNIT_MM) -
            gtk_page_setup_get_left_margin(setup, GTK_UNIT_MM) -
            gtk_page_setup_get_right_margin(setup, GTK_UNIT_MM);

        pd.paper_height_mm = gtk_page_setup_get_paper_height(setup, GTK_UNIT_MM) -
            gtk_page_setup_get_top_margin(setup, GTK_UNIT_MM) -
            gtk_page_setup_get_bottom_margin(setup, GTK_UNIT_MM);

        if ((gint)user_data & PRINT_TEMPLATE)
            read_print_template(template_in, context);
        else
            read_print_template(NULL, context);
        
        if ((gint)user_data & PRINT_ALL)
            find_print_judokas(NULL);
        
        if ((gint)user_data & PRINT_ONE_PER_PAGE)
            numpages = num_selected_judokas;
        else
            numpages = get_num_pages(&pd);

        gtk_print_operation_set_n_pages(operation, numpages);
    } else {
#if 0
        gint ctg = (gint)user_data;
        gint sys = db_get_system(ctg);

        if ((sys & SYSTEM_MASK) == SYSTEM_FRENCH_64)
            numpages = 3;
#endif
        gtk_print_operation_set_n_pages(operation, numpages);
    }
}

static void draw_page(GtkPrintOperation *operation,
                      GtkPrintContext   *context,
                      gint               page_nr,
                      gpointer           user_data)
{
    struct paint_data pd;
    gint ctg = (gint)user_data;
    GtkPageSetup *setup = gtk_print_context_get_page_setup(context);

    memset(&pd, 0, sizeof(pd));

    pd.category = ctg & PRINT_DATA_MASK;
    pd.c = gtk_print_context_get_cairo_context(context);
    pd.paper_width = gtk_print_context_get_width(context);
    pd.paper_height = gtk_print_context_get_height(context);

    pd.paper_width_mm = gtk_page_setup_get_paper_width(setup, GTK_UNIT_MM) -
        gtk_page_setup_get_left_margin(setup, GTK_UNIT_MM) -
        gtk_page_setup_get_right_margin(setup, GTK_UNIT_MM);

    pd.paper_height_mm = gtk_page_setup_get_paper_height(setup, GTK_UNIT_MM) -
        gtk_page_setup_get_top_margin(setup, GTK_UNIT_MM) -
        gtk_page_setup_get_bottom_margin(setup, GTK_UNIT_MM);

    if (print_landscape(pd.category)) {
        pd.rotate = TRUE;
    }

    switch (ctg & PRINT_ITEM_MASK) {
#if 0
    case PRINT_SHEET:
        current_page = page_nr;
        paint_category(c);
        break;
#endif
    case PRINT_WEIGHING_NOTES:
	paint_weight_notes(&pd, ctg, page_nr+1);
	break;	
    case PRINT_SCHEDULE:
        paint_schedule(&pd);
        break;
    case PRINT_SHEET:
    case PRINT_ALL_CATEGORIES:
        pd.category = pages_to_print[page_nr].cat;
        pd.page = pages_to_print[page_nr].pagenum;
        if (print_landscape(pd.category))
            pd.rotate = TRUE;
        paint_category(&pd);
        break;
    }

    //cairo_show_page(c);
    //cairo_destroy(c); do not destroy. its done by printing.
}

void do_print(GtkWidget *menuitem, gpointer userdata)
{
    GtkPrintOperation *print;
    GtkPrintOperationResult res;

    print = gtk_print_operation_new();

    g_signal_connect (print, "begin_print", G_CALLBACK (begin_print), userdata);
    g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), userdata);

    gtk_print_operation_set_use_full_page(print, FALSE);
    gtk_print_operation_set_unit(print, GTK_UNIT_POINTS);

    res = gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                  GTK_WINDOW (main_window), NULL);

    g_object_unref(print);
}

void print_doc(GtkWidget *menuitem, gpointer userdata)
{
    gchar *filename = NULL;
    cairo_surface_t *cs;
    cairo_t *c;
    struct judoka *cat = NULL;
    gint i;
        
    gint what  = (gint)userdata & PRINT_ITEM_MASK;
    gint where = (gint)userdata & PRINT_DEST_MASK;
    gint data  = (gint)userdata & PRINT_DATA_MASK;

    struct paint_data pd;
    memset(&pd, 0, sizeof(pd));
    //XXXpd.row_height = 1;
    pd.paper_width = SIZEX;
    pd.paper_height = SIZEY;
    pd.paper_width_mm = 210.0; // A4 paper
    pd.paper_height_mm = 297.0;

    switch (where) {
    case PRINT_TO_PRINTER:
        do_print(menuitem, userdata);
        break;
    case PRINT_TO_PDF:
        switch (what) {
        case PRINT_ALL_CATEGORIES:
            fill_in_pages(0, TRUE);
            filename = g_strdup("all.pdf");
            break;
        case PRINT_SHEET:
            data = fill_in_pages(data, FALSE);
            if (data > 0)
                cat = get_data(data);
            if (cat) {
                gchar *fn = g_strdup_printf("%s.pdf", cat->last);
                filename = get_save_as_name(fn, FALSE);
                free_judoka(cat);
                g_free(fn);
            } else {
                filename = get_save_as_name(_T(categoriesfile), FALSE);
            }
            break;
        case PRINT_WEIGHING_NOTES:
	    filename = g_strdup(pdf_out);
	    break;
        case PRINT_SCHEDULE:
            filename = get_save_as_name(_T(schedulefile), FALSE);
            break;
        }

        if (!filename)
            return;

        cs = cairo_pdf_surface_create(filename, SIZEX, SIZEY);
        c = pd.c = cairo_create(cs);

        switch (what) {
        case PRINT_ALL_CATEGORIES:
        case PRINT_SHEET:
            for (i = 0; i < numpages; i++) {
                pd.category = pages_to_print[i].cat;
                if (print_landscape(pd.category))
                    pd.rotate = TRUE;
                else
                    pd.rotate = FALSE;
                pd.page = pages_to_print[i].pagenum;
                paint_category(&pd);
                cairo_show_page(pd.c);
            }
            break;
        case PRINT_WEIGHING_NOTES:
	    if ((gint)userdata & PRINT_TEMPLATE)
		read_print_template(template_in, NULL);
	    else
		read_print_template(NULL, NULL);

	    if ((gint)userdata & PRINT_ALL)
		find_print_judokas(NULL);

	    if ((gint)userdata & PRINT_ONE_PER_PAGE)
		numpages = num_selected_judokas;
	    else
		numpages = get_num_pages(&pd);

            for (i = 1; i <= numpages; i++) {
                paint_weight_notes(&pd, (gint)userdata, i);
                cairo_show_page(pd.c);
            }
            break;
        case PRINT_SCHEDULE:
            paint_schedule(&pd);
            cairo_show_page(pd.c);
            break;
        }

        g_free(filename);
        cairo_destroy(c);
        cairo_surface_flush(cs);
        cairo_surface_destroy(cs);

        break;
    }
}

void print_matches(GtkWidget *menuitem, gpointer userdata)
{
    GtkWidget *dialog, *vbox, *not_started, *started, *finished;
    GtkWidget *p_result, *p_iwyks, *p_comment, *unmatched;
    GtkFileFilter *filter = gtk_file_filter_new();

    gtk_file_filter_add_pattern(filter, "*.csv");
    gtk_file_filter_set_name(filter, _("Matches"));

    dialog = gtk_file_chooser_dialog_new (_("Match File"),
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    vbox = gtk_vbox_new(FALSE, 1);
    gtk_widget_show(vbox);

    not_started = gtk_check_button_new_with_label(_("Print Not Started Categories"));
    gtk_widget_show(not_started);
    gtk_box_pack_start(GTK_BOX(vbox), not_started, FALSE, TRUE, 0);

    started = gtk_check_button_new_with_label(_("Print Started Categories"));
    gtk_widget_show(started);
    gtk_box_pack_start(GTK_BOX(vbox), started, FALSE, TRUE, 0);

    finished = gtk_check_button_new_with_label(_("Print Finished Categories"));
    gtk_widget_show(finished);
    gtk_box_pack_start(GTK_BOX(vbox), finished, FALSE, TRUE, 0);

    unmatched = gtk_check_button_new_with_label(_("Print Unmatched"));
    gtk_widget_show(unmatched);
    gtk_box_pack_start(GTK_BOX(vbox), unmatched, FALSE, TRUE, 0);

    p_result = gtk_check_button_new_with_label(_("Print Result"));
    gtk_widget_show(p_result);
    gtk_box_pack_start(GTK_BOX(vbox), p_result, FALSE, TRUE, 0);

    p_iwyks = gtk_check_button_new_with_label(_("Print IWYKS"));
    gtk_widget_show(p_iwyks);
    gtk_box_pack_start(GTK_BOX(vbox), p_iwyks, FALSE, TRUE, 0);

    p_comment = gtk_check_button_new_with_label(_("Print Comment"));
    gtk_widget_show(p_comment);
    gtk_box_pack_start(GTK_BOX(vbox), p_comment, FALSE, TRUE, 0);

    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), vbox);

    if (database_name[0] == 0) {
        if (current_directory[0] != '.')
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_directory);
        else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    } else {
        gchar *dirname = g_path_get_dirname(database_name);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    }

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *name, *name1;
        gint k;
        gboolean nst, st, fi, pr, pi, pc, um;

        nst = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(not_started));
        st = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(started));
        fi = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(finished));
        um = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(unmatched));
        pr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p_result));
        pi = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p_iwyks));
        pc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p_comment));

        name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        if (strstr(name, ".csv"))
            name1 = g_strdup_printf("%s", name);
        else
            name1 = g_strdup_printf("%s.csv", name);

        valid_ascii_string(name1);
                
        FILE *f = fopen(name1, "w");

        g_free (name);
        g_free (name1);

        if (!f)
            goto out;
                
        gchar *cmd = g_strdup_printf("select * from matches order by \"category\", \"number\"");
        gint numrows = db_get_table(cmd);
        g_free(cmd);
        
        if (numrows < 0) {
            fclose(f);
            goto out;
        }

        for (k = 0; k < numrows; k++) {
            struct judoka *j1, *j2, *c;

            gint deleted = atoi(db_get_data(k, "deleted"));
            if (deleted & DELETED)
                continue;

            gint cat = atoi(db_get_data(k, "category"));
 
            struct category_data *catdata = avl_get_category(cat);
            if (catdata) {
                gboolean ok = FALSE;
                if (nst && (catdata->match_status & MATCH_EXISTS) &&
                    (catdata->match_status & MATCH_MATCHED) == 0)
                    ok = TRUE;
                if (st && (catdata->match_status & MATCH_MATCHED) &&
                    (catdata->match_status & MATCH_UNMATCHED))
                    ok = TRUE;
                if (fi && (catdata->match_status & MATCH_MATCHED) &&
                    (catdata->match_status & MATCH_UNMATCHED) == 0)
                    ok = TRUE;
                if (ok == FALSE)
                    continue;
            }

            gint number = atoi(db_get_data(k, "number"));
            gint blue = atoi(db_get_data(k, "blue"));
            gint white = atoi(db_get_data(k, "white"));
            gint points_b = atoi(db_get_data(k, "blue_points"));
            gint points_w = atoi(db_get_data(k, "white_points"));
            gint score_b = atoi(db_get_data(k, "blue_score"));
            gint score_w = atoi(db_get_data(k, "white_score"));

            if (blue == GHOST || white == GHOST)
                continue;

            if (um && (points_b || points_w))
                continue;

            c = get_data(cat);
            j1 = get_data(blue);
            j2 = get_data(white);

            fprintf(f, "\"%s\",\"%d\",\"%s %s, %s\",\"%s %s, %s\"",
                    c?c->last:"", number,
                    j1?j1->first:"", j1?j1->last:"", j1?get_club_text(j1, 0):"",
                    j2?j2->first:"", j2?j2->last:"", j2?get_club_text(j2, 0):"");
            if (pr)
                fprintf(f, ",\"%d-%d\"", points_b, points_w);
            if (pi)
                fprintf(f, ",\"%05x-%05x\"", score_b, score_w);
            if (pc) {
                gchar *txt = get_match_number_text(cat, number);
                if (txt)
                    fprintf(f, ",\"%s\"", txt);
                else
                    fprintf(f, ",\"\"");
            }

            fprintf(f, "\r\n");

            free_judoka(c);
            free_judoka(j1);
            free_judoka(j2);
        }

        db_close_table();
        fclose(f);
    }

out:
    gtk_widget_destroy (dialog);
}

struct print_struct {
    GtkWidget *layout_default, *layout_template;
    GtkWidget *print_printer, *print_pdf;
    GtkWidget *pdf_file, *template_file;
};

static void update_print_struct(GtkWidget *w, gpointer *data)
{
    struct print_struct *s = data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->layout_default)))
        gtk_widget_set_sensitive(GTK_WIDGET(s->template_file), FALSE);
    else
        gtk_widget_set_sensitive(GTK_WIDGET(s->template_file), TRUE);
        
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->print_printer)))
        gtk_widget_set_sensitive(GTK_WIDGET(s->pdf_file), FALSE);
    else
        gtk_widget_set_sensitive(GTK_WIDGET(s->pdf_file), TRUE);
        
    gtk_button_set_label(GTK_BUTTON(s->template_file), template_in ? template_in : "");
    gtk_button_set_label(GTK_BUTTON(s->pdf_file), pdf_out ? pdf_out : "");
}

static void select_pdf(GtkWidget *w, GdkEventButton *event, gpointer *arg) 
{
    gchar *file = get_save_as_name("", FALSE);    
    if (file) {
        g_free(pdf_out);
        pdf_out = file;
        update_print_struct(NULL, arg);
    }
}

static void select_template(GtkWidget *w, GdkEventButton *event, gpointer *arg) 
{
    gchar *file = get_save_as_name("", TRUE);    
    if (file) {
        g_free(template_in);
        template_in = file;
        update_print_struct(NULL, arg);
    }
}

void print_accreditation_cards(gboolean all)
{
    GtkWidget *dialog;
    GtkWidget *table = gtk_table_new(3, 5, FALSE);
    GSList *layout_group = NULL;
    GSList *print_group = NULL;
    GtkWidget *one_per_page;
    struct print_struct *s;

    s = g_malloc(sizeof(*s));

    dialog = gtk_dialog_new_with_buttons (_("Print Accreditation Cards"),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                          NULL);

    gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new(_("Layout:")), 0, 1, 0, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new(_("Print To:")), 0, 1, 2, 4);

    s->layout_default = gtk_radio_button_new_with_label(layout_group, _("Default"));
    layout_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(s->layout_default));
    s->layout_template = gtk_radio_button_new_with_label(layout_group, _("Template"));

    gtk_table_attach_defaults(GTK_TABLE(table), s->layout_default, 1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), s->layout_template, 1, 2, 1, 2);

    s->print_printer = gtk_radio_button_new_with_label(print_group, _("Printer"));
    print_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(s->print_printer));
    s->print_pdf = gtk_radio_button_new_with_label(print_group, _("PDF"));

    gtk_table_attach_defaults(GTK_TABLE(table), s->print_printer, 1, 2, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table), s->print_pdf, 1, 2, 3, 4);

    one_per_page = gtk_check_button_new_with_label(_("One Card Per Page"));
    gtk_table_attach_defaults(GTK_TABLE(table), one_per_page, 0, 3, 4, 5);

    if (!template_in)
        template_in = g_build_filename(installation_dir, "etc", "note-template-example.txt", NULL);
    s->template_file = gtk_button_new_with_label(template_in);
    gtk_table_attach_defaults(GTK_TABLE(table), s->template_file, 2, 3, 1, 2);

    if (!pdf_out) {
        gchar *dirname = g_path_get_dirname(database_name);
        pdf_out = g_build_filename(dirname, "output.pdf", NULL);
        g_free(dirname);
    }
    s->pdf_file = gtk_button_new_with_label(pdf_out);
    gtk_table_attach_defaults(GTK_TABLE(table), s->pdf_file, 2, 3, 3, 4);

    gtk_widget_set_sensitive(GTK_WIDGET(s->template_file), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(s->pdf_file), FALSE);

    gtk_widget_show_all(table);
    gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), table);

    g_signal_connect(G_OBJECT(s->layout_default), "toggled", G_CALLBACK(update_print_struct), 
		     (gpointer)s);
    g_signal_connect(G_OBJECT(s->layout_template), "toggled", G_CALLBACK(update_print_struct), 
		     (gpointer)s);
    g_signal_connect(G_OBJECT(s->print_printer), "toggled", G_CALLBACK(update_print_struct), 
		     (gpointer)s);
    g_signal_connect(G_OBJECT(s->print_pdf), "toggled", G_CALLBACK(update_print_struct), 
		     (gpointer)s);
    g_signal_connect(G_OBJECT(s->pdf_file), "button-press-event", G_CALLBACK(select_pdf), 
		     (gpointer)s);
    g_signal_connect(G_OBJECT(s->template_file), "button-press-event", G_CALLBACK(select_template), 
		     (gpointer)s);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gint flags = PRINT_WEIGHING_NOTES;

        if (all)
            flags |= PRINT_ALL;

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(one_per_page)))
            flags |=  PRINT_ONE_PER_PAGE;

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->print_pdf)))
            flags |=  PRINT_TO_PDF;

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->layout_template)))
            flags |=  PRINT_TEMPLATE;

        print_doc(NULL, (gpointer)flags);
    }

    g_free(s);
    gtk_widget_destroy(dialog);
}

void print_weight_notes(GtkWidget *menuitem, gpointer userdata)
{
    print_accreditation_cards(TRUE);    
}
