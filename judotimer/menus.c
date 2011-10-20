/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2011 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "judotimer.h"

void start_help(GtkWidget *w, gpointer data);
void start_log_view(GtkWidget *w, gpointer data);

static void about_judotimer( GtkWidget *w,
                             gpointer   data )
{
    //GtkWidget *dialog;

    gtk_show_about_dialog (NULL,
                           "name", "Judotimer",
                           "title", _("About Judotimer"),
                           "copyright", "Copyright 2006-2011 Hannu Jokinen",
                           "version", SHIAI_VERSION,
                           "website", "http://sourceforge.net/projects/judoshiai/",
                           NULL);
}


static void match_auto(GtkWidget *w,
                       gpointer   data)
{
    clock_key(GDK_0, FALSE);
}

static void match_e_2(GtkWidget *w,
                      gpointer   data)
{
    //short_pin_times = GTK_CHECK_MENU_ITEM(w)->active;
    clock_key(GDK_1, FALSE);
}

static void match_2(GtkWidget *w,
                    gpointer   data)
{
    clock_key(GDK_2, FALSE);
}

static void match_3(GtkWidget *w,
                    gpointer   data)
{
    clock_key(GDK_3, FALSE);
}

static void match_4(GtkWidget *w,
                    gpointer   data)
{
    clock_key(GDK_4, FALSE);
}

static void match_5( GtkWidget *w,
                     gpointer   data )
{
    clock_key(GDK_5, FALSE);
}

static void gs_cb( GtkWidget *w,
		   gpointer   data )
{
    clock_key(GDK_9, FALSE);
}

static void tatami_selection(GtkWidget *w,
                             gpointer   data )
{
    tatami = (gint)data;
    g_key_file_set_integer(keyfile, "preferences", "tatami", tatami);
}

static void mode_selection(GtkWidget *w,
			   gpointer   data )
{
    mode = (gint)data;
}

static void display_competitors(GtkWidget *w,
                                gpointer   data )
{
    display_comp_window(saved_cat, saved_last1, saved_last2);
}

static GtkWidget *clock_min, *clock_sec, *osaekomi;

static void set_timers(GtkWidget *widget,
		       GdkEvent *event,
		       GtkWidget *data)
{
    if ((gulong)event == GTK_RESPONSE_OK) {
        set_clocks(atoi(gtk_entry_get_text(GTK_ENTRY(clock_min)))*60 +
                   atoi(gtk_entry_get_text(GTK_ENTRY(clock_sec))),
                   atoi(gtk_entry_get_text(GTK_ENTRY(osaekomi))));
    }
    gtk_widget_destroy(widget);
    clock_min = clock_sec = osaekomi = NULL;
}

static void manipulate_time(GtkWidget *w,
                            gpointer   data )
{
    gint what = (gint)data;
    switch (what) {
    case 0:
        hajime_inc_func();
        break;
    case 1:
        hajime_dec_func();
        break;
    case 2:
        osaekomi_inc_func();
        break;
    case 3:
        osaekomi_dec_func();
        break;
    case 4: {
        GtkWidget *dialog, *hbox, *label;

        dialog = gtk_dialog_new_with_buttons (_("Set clocks"),
                                              NULL,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_OK, GTK_RESPONSE_OK,
                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                              NULL);

        clock_min = gtk_entry_new();
        clock_sec = gtk_entry_new();
        osaekomi  = gtk_entry_new();
        gtk_entry_set_max_length(GTK_ENTRY(clock_min), 1);
        gtk_entry_set_max_length(GTK_ENTRY(clock_sec), 2);
        gtk_entry_set_max_length(GTK_ENTRY(osaekomi), 2);
        gtk_entry_set_width_chars(GTK_ENTRY(clock_min), 1);
        gtk_entry_set_width_chars(GTK_ENTRY(clock_sec), 2);
        gtk_entry_set_width_chars(GTK_ENTRY(osaekomi), 2);

        gtk_entry_set_text(GTK_ENTRY(clock_min), "0");
        gtk_entry_set_text(GTK_ENTRY(clock_sec), "00");
        gtk_entry_set_text(GTK_ENTRY(osaekomi), "00");

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);

        label = gtk_label_new(_("Clock:"));
        gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
        gtk_box_pack_start_defaults(GTK_BOX(hbox), clock_min);
        label = gtk_label_new(":");
        gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
        gtk_box_pack_start_defaults(GTK_BOX(hbox), clock_sec);
        label = gtk_label_new("Osaekomi:");
        gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
        gtk_box_pack_start_defaults(GTK_BOX(hbox), osaekomi);

        gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox);
        gtk_widget_show_all(dialog);

        g_signal_connect(G_OBJECT(dialog), "response",
                         G_CALLBACK(set_timers), NULL);
        break;
    }
    }
}

static void change_menu_label(GtkWidget *item, const gchar *new_text)
{
    GtkWidget *menu_label = gtk_bin_get_child(GTK_BIN(item));
    gtk_label_set_text(GTK_LABEL(menu_label), new_text);
}

#define NUM_NAME_LAYOUTS 11

static GtkWidget *menubar, *match, *preferences, *help, *matchmenu, *preferencesmenu, *helpmenu;
static GtkWidget *separator1, *separator2, *quit, *viewlog, *showcomp_act;
static GtkWidget *match0, *match1, *match2, *match3, *match4, *match5, *gs;
static GtkWidget *blue_wins, *white_wins, *red_background, *full_screen, *rules_no_koka;
static GtkWidget *rules_leave_points, *rules_stop_ippon, *whitefirst, *showcomp, *confirm_match;
static GtkWidget *tatami_sel, *tatami_sel_none, *tatami_sel_1,  *tatami_sel_2,  *tatami_sel_3,  *tatami_sel_4;
static GtkWidget *tatami_sel_5, *tatami_sel_6, *tatami_sel_7, *tatami_sel_8, *tatami_sel_9, *tatami_sel_10;
static GtkWidget *node_ip, *my_ip, *manual, *about, *quick_guide;
static GtkWidget *light, *menu_light, *menu_switched, *timeset;
static GtkWidget *inc_time, *dec_time, *inc_osaekomi, *dec_osaekomi, *clock_only, *set_time, *layout_sel;
static GtkWidget *layout_sel_1, *layout_sel_2, *layout_sel_3, *layout_sel_4, *layout_sel_5, *layout_sel_6, *layout_sel_7;
static GtkTooltips *menu_tips;
static GtkWidget *mode_normal, *mode_master, *mode_slave;
static GtkWidget *undo, *hansokumake_blue, *hansokumake_white, *clear_selection, *switch_sides;
static GtkWidget *advertise, *sound, *flags[NUM_LANGS], *menu_flags[NUM_LANGS];
static GtkWidget *name_layout, *name_layouts[NUM_NAME_LAYOUTS];

static const gchar *flags_files[NUM_LANGS] = {
    "finland.png", "sweden.png", "uk.png", "spain.png", "estonia.png", "ukraine.png", "iceland.png"
};

static const gchar *lang_names[NUM_LANGS] = {
    "fi", "sv", "en", "es", "et", "uk", "is"
};

static const gchar *help_file_names[NUM_LANGS] = {
    "judoshiai-fi.pdf", "judoshiai-en.pdf", "judoshiai-en.pdf", "judoshiai-es.pdf", "judoshiai-en.pdf",
    "judoshiai-uk.pdf", "judoshiai-en.pdf"
};

static const gchar *timer_help_file_names[NUM_LANGS] = {
    "judotimer-fi.pdf", "judotimer-en.pdf", "judotimer-en.pdf", "judotimer-es.pdf", "judotimer-en.pdf",
    "judotimer-uk.pdf", "judotimer-en.pdf"
};

static GtkWidget *get_picture(const gchar *name)
{
    gchar *file = g_build_filename(installation_dir, "etc", name, NULL);
    GtkWidget *pic = gtk_image_new_from_file(file);
    g_free(file);
    return pic;
}

static void set_menu_item_picture(GtkImageMenuItem *menu_item, gchar *name)
{
    GtkImage *image = (GtkImage *)gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(menu_item));
    gchar *file = g_build_filename(installation_dir, "etc", name, NULL);
    gtk_image_set_from_file(image, file);
    g_free(file);
}

static gint light_callback(gpointer data)
{
    extern gboolean connection_ok;
    extern time_t traffic_last_rec_time;
    static gboolean last_ok = FALSE;
    static gboolean yellow_set = FALSE;

    if (yellow_set == FALSE && connection_ok && time(NULL) > traffic_last_rec_time + 6) {
        set_menu_item_picture(GTK_IMAGE_MENU_ITEM(menu_light), "yellowlight.png");
        yellow_set = TRUE;
        return TRUE;
    } else if (yellow_set && connection_ok && time(NULL) < traffic_last_rec_time + 6) {
        yellow_set = FALSE;
        last_ok = !connection_ok;
    }

    if (connection_ok == last_ok)
        return TRUE;

    last_ok = connection_ok;

    if (connection_ok)
        set_menu_item_picture(GTK_IMAGE_MENU_ITEM(menu_light), "greenlight.png");
    else
        set_menu_item_picture(GTK_IMAGE_MENU_ITEM(menu_light), "redlight.png");

    return TRUE;
}

void clear_switch_sides(void)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(switch_sides), FALSE);
    toggle_switch_sides(switch_sides, NULL);
}

void light_switch_sides(gboolean yes)
{
    if (yes)
        set_menu_item_picture(GTK_IMAGE_MENU_ITEM(menu_switched), "switched.png");
    else
        set_menu_item_picture(GTK_IMAGE_MENU_ITEM(menu_switched), "notswitched.png");
}

/* Returns a menubar widget made from the above menu */
GtkWidget *get_menubar_menu(GtkWidget  *window)
{
    gint i;
    GtkAccelGroup *group;
    GtkWidget *submenu;

    menu_tips = gtk_tooltips_new ();
    group = gtk_accel_group_new ();
    menubar = gtk_menu_bar_new ();

    match       = gtk_menu_item_new_with_label (_("Match"));
    preferences = gtk_menu_item_new_with_label (_("Preferences"));
    help        = gtk_menu_item_new_with_label (_("Help"));
    undo        = gtk_menu_item_new_with_label (_("Undo!"));

    for (i = 0; i < NUM_LANGS; i++) {
        flags[i] = get_picture(flags_files[i]);
        menu_flags[i] = gtk_image_menu_item_new();
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_flags[i]), flags[i]);        
        gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(menu_flags[i]), TRUE);
    }

    light      = get_picture("redlight.png");
    menu_light = gtk_image_menu_item_new();
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_light), light);
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(menu_light), TRUE);

    menu_switched = gtk_image_menu_item_new();
    light      = get_picture("notswitched.png");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_switched), light);
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(menu_switched), TRUE);

    matchmenu        = gtk_menu_new ();
    preferencesmenu  = gtk_menu_new ();
    helpmenu         = gtk_menu_new ();

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (match), matchmenu);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (preferences), preferencesmenu);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (help), helpmenu);

    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), match);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), preferences);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), help);

    for (i = 0; i < NUM_LANGS; i++) {
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_flags[i]); 
        g_signal_connect(G_OBJECT(menu_flags[i]), "button_press_event",
                         G_CALLBACK(change_language), (gpointer)i);
    }

    //gtk_menu_shell_append (GTK_MENU_SHELL (menubar), undo);
    g_signal_connect(G_OBJECT(undo), "button_press_event",
                     G_CALLBACK(undo_func), (gpointer)NULL);


    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menu_switched);
    gtk_menu_item_set_right_justified(GTK_MENU_ITEM(menu_switched), TRUE);

    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menu_light);
    g_signal_connect(G_OBJECT(menu_light), "button_press_event",
                     G_CALLBACK(ask_node_ip_address), (gpointer)NULL);
    gtk_menu_item_set_right_justified(GTK_MENU_ITEM(menu_light), TRUE);

    /* Create the Contest menu content. */
    match0 = gtk_menu_item_new_with_label(_("Match duration: automatic"));
    //match1 = gtk_check_menu_item_new_with_label(_("Short pin times"));
    match1 = gtk_menu_item_new_with_label(_("Match duration: 2 min E-juniors"));
    match2 = gtk_menu_item_new_with_label(_("Match duration: 2 min"));
    match3 = gtk_menu_item_new_with_label(_("Match duration: 3 min"));
    match4 = gtk_menu_item_new_with_label(_("Match duration: 4 min"));
    match5 = gtk_menu_item_new_with_label(_("Match duration: 5 min"));
    gs     = gtk_menu_item_new_with_label(_("Golden Score"));
    separator1 = gtk_separator_menu_item_new();
    blue_wins = gtk_menu_item_new_with_label(_("Hantei: blue wins"));
    white_wins = gtk_menu_item_new_with_label(_("Hantei: white wins"));
    hansokumake_blue = gtk_menu_item_new_with_label(_("Hansoku-make to blue"));
    hansokumake_white = gtk_menu_item_new_with_label(_("Hansoku-make to white"));
    clear_selection = gtk_menu_item_new_with_label(_("Clear selection"));
    switch_sides  = gtk_check_menu_item_new_with_label("Switch sides");
    separator2 = gtk_separator_menu_item_new();
    viewlog = gtk_menu_item_new_with_label(_("View Log"));
    showcomp_act = gtk_menu_item_new_with_label(_("Show Competitors"));
    quit = gtk_menu_item_new_with_label(_("Quit"));

    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), match0);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), match1);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), match2);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), match3);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), match4);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), match5);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), gs);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), separator1);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), blue_wins);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), white_wins);
    //gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), hansokumake_blue);
    //gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), hansokumake_white);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), clear_selection);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), switch_sides);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), separator2);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), viewlog);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), showcomp_act);
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append (GTK_MENU_SHELL (matchmenu), quit);

    g_signal_connect(G_OBJECT(match0),      "activate", G_CALLBACK(match_auto), 0);
    g_signal_connect(G_OBJECT(match1),      "activate", G_CALLBACK(match_e_2), 0);
    g_signal_connect(G_OBJECT(match2),      "activate", G_CALLBACK(match_2), 0);
    g_signal_connect(G_OBJECT(match3),      "activate", G_CALLBACK(match_3), 0);
    g_signal_connect(G_OBJECT(match4),      "activate", G_CALLBACK(match_4), 0);
    g_signal_connect(G_OBJECT(match5),      "activate", G_CALLBACK(match_5), 0);
    g_signal_connect(G_OBJECT(gs),          "activate", G_CALLBACK(gs_cb), 0);
    g_signal_connect(G_OBJECT(blue_wins),   "activate", G_CALLBACK(voting_result), (gpointer)HANTEI_BLUE);
    g_signal_connect(G_OBJECT(white_wins),  "activate", G_CALLBACK(voting_result), (gpointer)HANTEI_WHITE);
    g_signal_connect(G_OBJECT(hansokumake_blue), "activate", G_CALLBACK(voting_result), (gpointer)HANSOKUMAKE_BLUE);
    g_signal_connect(G_OBJECT(hansokumake_white), "activate", G_CALLBACK(voting_result), (gpointer)HANSOKUMAKE_WHITE);
    g_signal_connect(G_OBJECT(clear_selection), "activate", G_CALLBACK(voting_result), (gpointer)CLEAR_SELECTION);
    g_signal_connect(G_OBJECT(switch_sides), "activate", G_CALLBACK(toggle_switch_sides), (gpointer)0);

    g_signal_connect(G_OBJECT(viewlog),     "activate", G_CALLBACK(start_log_view), NULL);
    g_signal_connect(G_OBJECT(showcomp_act),"activate", G_CALLBACK(display_competitors), NULL);
    g_signal_connect(G_OBJECT(quit),        "activate", G_CALLBACK(destroy/*gtk_main_quit*/), NULL);

    gtk_widget_add_accelerator(quit, "activate", group, GDK_Q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(match0, "activate", group, GDK_0, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(match1, "activate", group, GDK_1, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(match2, "activate", group, GDK_2, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(match3, "activate", group, GDK_3, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(match4, "activate", group, GDK_4, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(match5, "activate", group, GDK_5, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(gs,     "activate", group, GDK_9, 0, GTK_ACCEL_VISIBLE);

    /* Create the Preferences menu content. */
    red_background  = gtk_check_menu_item_new_with_label("Red background");
    whitefirst      = gtk_check_menu_item_new_with_label("White first");
    showcomp        = gtk_check_menu_item_new_with_label("");
    clock_only      = gtk_check_menu_item_new_with_label("View only clocks");
    layout_sel_1    = gtk_radio_menu_item_new_with_label(NULL, "");
    layout_sel_2    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)layout_sel_1, "");
    layout_sel_3    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)layout_sel_1, "");
    layout_sel_4    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)layout_sel_1, "");
    layout_sel_5    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)layout_sel_1, "");
    layout_sel_6    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)layout_sel_1, "");
    layout_sel_7    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)layout_sel_1, "");

    name_layouts[0] = gtk_radio_menu_item_new_with_label(NULL, "");
    for (i = 1; i < NUM_NAME_LAYOUTS; i++)
        name_layouts[i] = gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(name_layouts[0]), "");

    full_screen     = gtk_check_menu_item_new_with_label("Full screen mode");
    rules_no_koka   = gtk_check_menu_item_new_with_label("");
    rules_leave_points = gtk_check_menu_item_new_with_label("");
    rules_stop_ippon = gtk_check_menu_item_new_with_label("");
    confirm_match   = gtk_check_menu_item_new_with_label("");
    tatami_sel_none = gtk_radio_menu_item_new_with_label(NULL, "");
    tatami_sel_1    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    tatami_sel_2    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    tatami_sel_3    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    tatami_sel_4    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    tatami_sel_5    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    tatami_sel_6    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    tatami_sel_7    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    tatami_sel_8    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    tatami_sel_9    = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    tatami_sel_10   = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)tatami_sel_none, "");
    node_ip         = gtk_menu_item_new_with_label("Communication node");
    my_ip           = gtk_menu_item_new_with_label("Own IP addresses");
    inc_time        = gtk_menu_item_new_with_label("");
    dec_time        = gtk_menu_item_new_with_label("");
    inc_osaekomi    = gtk_menu_item_new_with_label("");
    dec_osaekomi    = gtk_menu_item_new_with_label("");
    set_time        = gtk_menu_item_new_with_label("");
    mode_normal     = gtk_radio_menu_item_new_with_label(NULL, "");
    mode_master     = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)mode_normal, "");
    mode_slave      = gtk_radio_menu_item_new_with_label_from_widget((GtkRadioMenuItem *)mode_normal, "");
    advertise       = gtk_menu_item_new_with_label("");
    sound           = gtk_menu_item_new_with_label("");


    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), red_background);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), full_screen);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), rules_no_koka);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), rules_leave_points);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), rules_stop_ippon);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), confirm_match);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), whitefirst);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), showcomp);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), gtk_separator_menu_item_new());

    layout_sel = gtk_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(preferencesmenu), layout_sel);
    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(layout_sel), submenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), layout_sel_1);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), layout_sel_2);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), layout_sel_3);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), layout_sel_4);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), layout_sel_6);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), layout_sel_7);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), layout_sel_5);

    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), gtk_separator_menu_item_new());

    name_layout = gtk_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(preferencesmenu), name_layout);
    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(name_layout), submenu);
    for (i = 0; i < NUM_NAME_LAYOUTS; i++)
        gtk_menu_shell_append (GTK_MENU_SHELL (submenu), name_layouts[i]);

    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), gtk_separator_menu_item_new());

    tatami_sel = gtk_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(preferencesmenu), tatami_sel);
    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(tatami_sel), submenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_none);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_1);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_2);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_3);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_4);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_5);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_6);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_7);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_8);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_9);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tatami_sel_10);

    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), node_ip);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), my_ip);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), gtk_separator_menu_item_new());

    timeset = gtk_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(preferencesmenu), timeset);
    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(timeset), submenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), inc_time);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), dec_time);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), inc_osaekomi);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), dec_osaekomi);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), set_time);

    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), mode_normal);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), mode_master);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), mode_slave);

    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), advertise);
    gtk_menu_shell_append (GTK_MENU_SHELL (preferencesmenu), sound);

    g_signal_connect(G_OBJECT(red_background),  "activate", G_CALLBACK(toggle_color),         (gpointer)1);
    g_signal_connect(G_OBJECT(full_screen),     "activate", G_CALLBACK(toggle_full_screen),   (gpointer)0);
    g_signal_connect(G_OBJECT(rules_no_koka),      "activate", G_CALLBACK(toggle_rules_no_koka),    (gpointer)0);
    g_signal_connect(G_OBJECT(rules_leave_points), "activate", G_CALLBACK(toggle_rules_leave_points), (gpointer)0);
    g_signal_connect(G_OBJECT(rules_stop_ippon), "activate", G_CALLBACK(toggle_rules_stop_ippon), (gpointer)0);
    g_signal_connect(G_OBJECT(confirm_match),   "activate", G_CALLBACK(toggle_confirm_match),  (gpointer)0);
    g_signal_connect(G_OBJECT(whitefirst),      "activate", G_CALLBACK(toggle_whitefirst),     (gpointer)0);
    g_signal_connect(G_OBJECT(showcomp),        "activate", G_CALLBACK(toggle_show_comp),      (gpointer)0);
    g_signal_connect(G_OBJECT(layout_sel_1),    "activate", G_CALLBACK(select_display_layout), (gpointer)1);
    g_signal_connect(G_OBJECT(layout_sel_2),    "activate", G_CALLBACK(select_display_layout), (gpointer)2);
    g_signal_connect(G_OBJECT(layout_sel_3),    "activate", G_CALLBACK(select_display_layout), (gpointer)3);
    g_signal_connect(G_OBJECT(layout_sel_4),    "activate", G_CALLBACK(select_display_layout), (gpointer)4);
    g_signal_connect(G_OBJECT(layout_sel_5),    "activate", G_CALLBACK(select_display_layout), (gpointer)5);
    g_signal_connect(G_OBJECT(layout_sel_6),    "activate", G_CALLBACK(select_display_layout), (gpointer)6);
    g_signal_connect(G_OBJECT(layout_sel_7),    "activate", G_CALLBACK(select_display_layout), (gpointer)7);

    for (i = 0; i < NUM_NAME_LAYOUTS; i++)
        g_signal_connect(G_OBJECT(name_layouts[i]), "activate", G_CALLBACK(select_name_layout), (gpointer)i);

    g_signal_connect(G_OBJECT(tatami_sel_none), "activate", G_CALLBACK(tatami_selection),     (gpointer)0);
    g_signal_connect(G_OBJECT(tatami_sel_1),    "activate", G_CALLBACK(tatami_selection),     (gpointer)1);
    g_signal_connect(G_OBJECT(tatami_sel_2),    "activate", G_CALLBACK(tatami_selection),     (gpointer)2);
    g_signal_connect(G_OBJECT(tatami_sel_3),    "activate", G_CALLBACK(tatami_selection),     (gpointer)3);
    g_signal_connect(G_OBJECT(tatami_sel_4),    "activate", G_CALLBACK(tatami_selection),     (gpointer)4);
    g_signal_connect(G_OBJECT(tatami_sel_5),    "activate", G_CALLBACK(tatami_selection),     (gpointer)5);
    g_signal_connect(G_OBJECT(tatami_sel_6),    "activate", G_CALLBACK(tatami_selection),     (gpointer)6);
    g_signal_connect(G_OBJECT(tatami_sel_7),    "activate", G_CALLBACK(tatami_selection),     (gpointer)7);
    g_signal_connect(G_OBJECT(tatami_sel_8),    "activate", G_CALLBACK(tatami_selection),     (gpointer)8);
    g_signal_connect(G_OBJECT(tatami_sel_9),    "activate", G_CALLBACK(tatami_selection),     (gpointer)9);
    g_signal_connect(G_OBJECT(tatami_sel_10),    "activate", G_CALLBACK(tatami_selection),    (gpointer)10);
    g_signal_connect(G_OBJECT(node_ip),         "activate", G_CALLBACK(ask_node_ip_address),  (gpointer)0);
    g_signal_connect(G_OBJECT(my_ip),           "activate", G_CALLBACK(show_my_ip_addresses), (gpointer)0);
    g_signal_connect(G_OBJECT(inc_time),        "activate", G_CALLBACK(manipulate_time),      (gpointer)0);
    g_signal_connect(G_OBJECT(dec_time),        "activate", G_CALLBACK(manipulate_time),      (gpointer)1);
    g_signal_connect(G_OBJECT(inc_osaekomi),    "activate", G_CALLBACK(manipulate_time),      (gpointer)2);
    g_signal_connect(G_OBJECT(dec_osaekomi),    "activate", G_CALLBACK(manipulate_time),      (gpointer)3);
    g_signal_connect(G_OBJECT(set_time),        "activate", G_CALLBACK(manipulate_time),      (gpointer)4);
    g_signal_connect(G_OBJECT(mode_normal),     "activate", G_CALLBACK(mode_selection),       (gpointer)0);
    g_signal_connect(G_OBJECT(mode_master),     "activate", G_CALLBACK(mode_selection),       (gpointer)1);
    g_signal_connect(G_OBJECT(mode_slave),      "activate", G_CALLBACK(mode_selection),       (gpointer)2);
    g_signal_connect(G_OBJECT(advertise),       "activate", G_CALLBACK(toggle_advertise),     (gpointer)0);
    g_signal_connect(G_OBJECT(sound),           "activate", G_CALLBACK(select_sound),         (gpointer)0);

    gtk_widget_add_accelerator(full_screen,     "activate", group, GDK_F, 0, GTK_ACCEL_VISIBLE);

    /* Create the Help menu content. */
    manual = gtk_menu_item_new_with_label("Manual");
    quick_guide = gtk_menu_item_new_with_label("Quick guide");
    about = gtk_menu_item_new_with_label("About");

    gtk_menu_shell_append (GTK_MENU_SHELL (helpmenu), manual);
    gtk_menu_shell_append (GTK_MENU_SHELL (helpmenu), quick_guide);
    gtk_menu_shell_append (GTK_MENU_SHELL (helpmenu), about);

    g_signal_connect(G_OBJECT(manual),          "activate", G_CALLBACK(start_help),            (gpointer)0);
    g_signal_connect(G_OBJECT(quick_guide),     "activate", G_CALLBACK(start_help),            (gpointer)1);
    g_signal_connect(G_OBJECT(about),           "activate", G_CALLBACK(about_judotimer),       (gpointer)0);

    /* Attach the new accelerator group to the window. */
    gtk_window_add_accel_group (GTK_WINDOW (window), group);

    g_timeout_add(1000, light_callback, NULL);

    /* Finally, return the actual menu bar created by the item factory. */
    return menubar;
}

void set_preferences(void)
{
    GError *error = NULL;
    gchar  *str;
    gint    i;

    if ((str = g_key_file_get_string(keyfile, "preferences", "color", &error))) {
        if (strcmp(str, "red") == 0) {
            gtk_menu_item_activate(GTK_MENU_ITEM(red_background));
            //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(red_background), TRUE);
            //toggle_color(red_background, NULL);
        }
        g_free(str);
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "fullscreen", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(full_screen));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "rulesnokoka", &error) || error) {
        gtk_menu_item_activate(GTK_MENU_ITEM(rules_no_koka));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "rulesleavepoints", &error) || error) {
        gtk_menu_item_activate(GTK_MENU_ITEM(rules_leave_points));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "stopippon", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(rules_stop_ippon));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "confirmmatch", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(confirm_match));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "whitefirst", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(whitefirst));
    }

    error = NULL;
    i = g_key_file_get_integer(keyfile, "preferences", "tatami", &error);
    if (!error) {
        switch (i) {
        case 0: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_none)); break;
        case 1: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_1)); break;
        case 2: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_2)); break;
        case 3: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_3)); break;
        case 4: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_4)); break;
        case 5: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_5)); break;
        case 6: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_6)); break;
        case 7: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_7)); break;
        case 8: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_8)); break;
        case 9: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_9)); break;
        case 10: gtk_menu_item_activate(GTK_MENU_ITEM(tatami_sel_10)); break;
        }
    }

    error = NULL;
    i = g_key_file_get_integer(keyfile, "preferences", "displaylayout", &error);
    if (!error) {
        switch (i) {
        case 1: gtk_menu_item_activate(GTK_MENU_ITEM(layout_sel_1)); break;
        case 2: gtk_menu_item_activate(GTK_MENU_ITEM(layout_sel_2)); break;
        case 3: gtk_menu_item_activate(GTK_MENU_ITEM(layout_sel_3)); break;
        case 4: gtk_menu_item_activate(GTK_MENU_ITEM(layout_sel_4)); break;
        case 5: gtk_menu_item_activate(GTK_MENU_ITEM(layout_sel_5)); break;
        case 6: gtk_menu_item_activate(GTK_MENU_ITEM(layout_sel_6)); break;
        case 7: gtk_menu_item_activate(GTK_MENU_ITEM(layout_sel_7)); break;
        }
    } else {
        select_display_layout(NULL, (gpointer)1);
    }

    error = NULL;
    i = g_key_file_get_integer(keyfile, "preferences", "namelayout", &error);
    if (!error)
        gtk_menu_item_activate(GTK_MENU_ITEM(name_layouts[i]));
    else 
        gtk_menu_item_activate(GTK_MENU_ITEM(name_layouts[0]));

    error = NULL;
    str = g_key_file_get_string(keyfile, "preferences", "nodeipaddress", &error);
    if (!error) {
        gulong a,b,c,d;
        sscanf(str, "%ld.%ld.%ld.%ld", &a, &b, &c, &d);
        node_ip_addr = host2net((a << 24) | (b << 16) | (c << 8) | d);
        g_free(str);
    }

    error = NULL;
    i = g_key_file_get_integer(keyfile, "preferences", "language", &error);
    if (!error)
        language = i;
    else
        language = LANG_FI;

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "showcompetitornames", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(showcomp));
        show_competitor_names = TRUE;
    }

}

gboolean change_language(GtkWidget *eventbox, GdkEventButton *event, void *param)
{
    gchar *r = NULL;
    static gchar envbuf[32]; // this must be static for the putenv() function

    language = (gint)param;
    sprintf(envbuf, "LANGUAGE=%s", lang_names[language]);
    putenv(envbuf);
    r = setlocale(LC_ALL, lang_names[language]);

    gchar *dirname = g_build_filename(installation_dir, "share", "locale", NULL);
    bindtextdomain ("judoshiai", dirname);
    g_free(dirname);
    bind_textdomain_codeset ("judoshiai", "UTF-8");
    textdomain ("judoshiai");

    change_menu_label(match,        _("Contest"));
    change_menu_label(preferences,  _("Preferences"));
    change_menu_label(help,         _("Help"));
    change_menu_label(undo,         _("Undo!"));

    change_menu_label(match0, _("Contest duration: automatic"));
    change_menu_label(match1, _("Contest duration: 2 min (short pin times)"));
    change_menu_label(match2, _("Contest duration: 2 min"));
    change_menu_label(match3, _("Contest duration: 3 min"));
    change_menu_label(match4, _("Contest duration: 4 min"));
    change_menu_label(match5, _("Contest duration: 5 min"));
    change_menu_label(gs,     _("Golden Score"));

    if (white_first) {
        change_menu_label(blue_wins,         _("Hantei: white wins"));
        change_menu_label(white_wins,        _("Hantei: blue wins"));
    } else {
        change_menu_label(blue_wins,         _("Hantei: blue wins"));
        change_menu_label(white_wins,        _("Hantei: white wins"));
    }
    if (white_first) {
        change_menu_label(hansokumake_blue,  _("Hansoku-make to white"));
        change_menu_label(hansokumake_white, _("Hansoku-make to blue"));
    } else {
        change_menu_label(hansokumake_blue,  _("Hansoku-make to blue"));
        change_menu_label(hansokumake_white, _("Hansoku-make to white"));
    }
    change_menu_label(clear_selection,   _("Clear selection"));
    change_menu_label(switch_sides,      _("Switch sides"));

    change_menu_label(viewlog,      _("View Log"));
    change_menu_label(showcomp_act, _("Show Competitors"));
    change_menu_label(quit,         _("Quit"));

    change_menu_label(red_background, _("Red background"));
    change_menu_label(full_screen, _("Full screen mode"));
    change_menu_label(rules_no_koka, _("No koka"));
    change_menu_label(rules_leave_points, _("Leave points for GS"));
    change_menu_label(rules_stop_ippon, _("Stop clock on Ippon"));
    change_menu_label(confirm_match, _("Confirm New Match"));
    change_menu_label(clock_only, _("View clocks only"));
    change_menu_label(whitefirst, _("White first"));
    change_menu_label(showcomp, _("Show competitors"));

    change_menu_label(layout_sel,   _("Display layout"));
    change_menu_label(layout_sel_1, _("Display layout 1"));
    change_menu_label(layout_sel_2, _("Display layout 2"));
    change_menu_label(layout_sel_3, _("Display layout 3"));
    change_menu_label(layout_sel_4, _("Display layout 4"));
    change_menu_label(layout_sel_5, _("View clocks only"));
    change_menu_label(layout_sel_6, _("Display layout 5"));
    change_menu_label(layout_sel_7, _("Display customized layout"));

    change_menu_label(name_layout,   _("Name format"));
    change_menu_label(name_layouts[0], _("Name Surname, Country/Club"));
    change_menu_label(name_layouts[1], _("Surname, Name, Country/Club"));
    change_menu_label(name_layouts[2], _("Country/Club  Surname, Name"));
    change_menu_label(name_layouts[3], _("Country  Surname, Name"));
    change_menu_label(name_layouts[4], _("Club  Surname, Name"));
    change_menu_label(name_layouts[5], _("Country Surname"));
    change_menu_label(name_layouts[6], _("Club Surname"));
    change_menu_label(name_layouts[7], _("Surname, Name"));
    change_menu_label(name_layouts[8], _("Surname"));
    change_menu_label(name_layouts[9], _("Country"));
    change_menu_label(name_layouts[10], _("Club"));

    change_menu_label(tatami_sel,   _("Contest area"));
    change_menu_label(tatami_sel_none, _("Contest area not chosen"));
    change_menu_label(tatami_sel_1, _("Contest area 1"));
    change_menu_label(tatami_sel_2, _("Contest area 2"));
    change_menu_label(tatami_sel_3, _("Contest area 3"));
    change_menu_label(tatami_sel_4, _("Contest area 4"));
    change_menu_label(tatami_sel_5, _("Contest area 5"));
    change_menu_label(tatami_sel_6, _("Contest area 6"));
    change_menu_label(tatami_sel_7, _("Contest area 7"));
    change_menu_label(tatami_sel_8, _("Contest area 8"));
    change_menu_label(tatami_sel_9, _("Contest area 9"));
    change_menu_label(tatami_sel_10, _("Contest area 10"));

    change_menu_label(node_ip,      _("Communication node"));
    change_menu_label(my_ip,        _("Own IP addresses"));

    change_menu_label(timeset,      _("Set time"));
    change_menu_label(inc_time,     _("Increment time"));
    change_menu_label(dec_time,     _("Decrement time"));
    change_menu_label(inc_osaekomi, _("Increment pin time"));
    change_menu_label(dec_osaekomi, _("Decrement pin time"));
    change_menu_label(set_time,     _("Set clocks"));

    change_menu_label(mode_normal, _("Normal operation"));
    change_menu_label(mode_master, _("Master mode"));
    change_menu_label(mode_slave,  _("Slave mode"));

    change_menu_label(advertise,    _("Advertise"));
    change_menu_label(sound,        _("Sound"));

    change_menu_label(manual,       _("Manual"));
    change_menu_label(quick_guide,  _("Quick guide"));
    change_menu_label(about,        _("About"));

    /* tooltips */
    gtk_tooltips_set_tip (GTK_TOOLTIPS (menu_tips), menu_flags[LANG_FI],
                          _("Change language to Finnish"), NULL);
    gtk_tooltips_set_tip (GTK_TOOLTIPS (menu_tips), menu_flags[LANG_SW],
                          _("Change language to Swedish"), NULL);
    gtk_tooltips_set_tip (GTK_TOOLTIPS (menu_tips), menu_flags[LANG_EN],
                          _("Change language to English"), NULL);
    gtk_tooltips_set_tip (GTK_TOOLTIPS (menu_tips), menu_flags[LANG_ES],
                          _("Change language to Spanish"), NULL);
    gtk_tooltips_set_tip (GTK_TOOLTIPS (menu_tips), menu_flags[LANG_EE],
                          _("Change language to Estonian"), NULL);
    gtk_tooltips_set_tip (GTK_TOOLTIPS (menu_tips), menu_flags[LANG_UK],
                          _("Change language to Ukrainan"), NULL);
    gtk_tooltips_set_tip (GTK_TOOLTIPS (menu_tips), menu_flags[LANG_IS],
                          _("Change language to Icelandic"), NULL);

    gtk_tooltips_set_tip (GTK_TOOLTIPS (menu_tips), match0,
                          _("Contest duration automatically from JudoShiai program"), NULL);

    change_language_1();

    g_key_file_set_integer(keyfile, "preferences", "language", language);

    return TRUE;
}

#if 0

static gint nmenu_items = sizeof (menu_items_en) / sizeof (menu_items_en[0]);

/* Returns a menubar widget made from the above menu */
GtkWidget *get_menubar_menu_en( GtkWidget  *window )
{
    GtkItemFactory *item_factory;
    GtkAccelGroup *accel_group;

    /* Make an accelerator group (shortcut keys) */
    accel_group = gtk_accel_group_new ();

    /* Make an ItemFactory (that makes a menubar) */
    item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
                                         accel_group);

    /* This function generates the menu items. Pass the item factory,
       the number of items in the array, the array itself, and any
       callback data for the the menu items. */
    gtk_item_factory_create_items (item_factory, nmenu_items, menu_items_en, NULL);

    /* Attach the new accelerator group to the window. */
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    /* Finally, return the actual menu bar created by the item factory. */
    return gtk_item_factory_get_widget (item_factory, "<main>");
}

GtkWidget *get_menubar_menu_fi( GtkWidget  *window )
{
    GtkItemFactory *item_factory;
    GtkAccelGroup *accel_group;

    /* Make an accelerator group (shortcut keys) */
    accel_group = gtk_accel_group_new ();

    /* Make an ItemFactory (that makes a menubar) */
    item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
                                         accel_group);

    /* This function generates the menu items. Pass the item factory,
       the number of items in the array, the array itself, and any
       callback data for the the menu items. */
    gtk_item_factory_create_items (item_factory, nmenu_items, menu_items_fi, NULL);

    /* Attach the new accelerator group to the window. */
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    /* Finally, return the actual menu bar created by the item factory. */
    return gtk_item_factory_get_widget (item_factory, "<main>");
}

/* Popup the menu when the popup button is pressed */
static gboolean popup_cb( GtkWidget *widget,
                          GdkEvent *event,
                          GtkWidget *menu )
{
    GdkEventButton *bevent = (GdkEventButton *)event;

    /* Only take button presses */
    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    /* Show the menu */
    gtk_menu_popup (GTK_MENU(menu), NULL, NULL,
                    NULL, NULL, bevent->button, bevent->time);

    return TRUE;
}

/* Same as with get_menubar_menu() but just return a button with a signal to
   call a popup menu */
GtkWidget *get_popup_menu( void )
{
    GtkItemFactory *item_factory;
    GtkWidget *button, *menu;

    /* Same as before but don't bother with the accelerators */
    item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>",
                                         NULL);
    gtk_item_factory_create_items (item_factory, nmenu_items, menu_items_en, NULL);
    menu = gtk_item_factory_get_widget (item_factory, "<main>");

    /* Make a button to activate the popup menu */
    button = gtk_button_new_with_label ("Popup");
    /* Make the menu popup when clicked */
    g_signal_connect (G_OBJECT(button),
                      "event",
                      G_CALLBACK(popup_cb),
                      (gpointer) menu);

    return button;
}

/* Same again but return an option menu */
GtkWidget *get_option_menu( void )
{
    GtkItemFactory *item_factory;
    GtkWidget *option_menu;

    /* Same again, not bothering with the accelerators */
    item_factory = gtk_item_factory_new (GTK_TYPE_OPTION_MENU, "<main>",
                                         NULL);
    gtk_item_factory_create_items (item_factory, nmenu_items, menu_items_en, NULL);
    option_menu = gtk_item_factory_get_widget (item_factory, "<main>");

    return option_menu;
}
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 1
#include "windows.h"
#include "shellapi.h"
#endif /* WIN32 */

void start_help(GtkWidget *w, gpointer data)
{
    gchar *docfile = g_build_filename(installation_dir, "doc",
                                      data ?
                                      timer_help_file_names[language] :
                                      help_file_names[language], NULL);
#ifdef WIN32
    ShellExecute(NULL, TEXT("open"), docfile, NULL, ".\\", SW_SHOWMAXIMIZED);
#else /* ! WIN32 */
    gchar *cmd;
    cmd = g_strdup_printf("if which acroread ; then PDFR=acroread ; "
                          "elif which evince ; then PDFR=evince ; "
                          "else PDFR=xpdf ; fi ; $PDFR \"%s\" &", docfile);
    system(cmd);
    g_free(cmd);
#endif /* ! WIN32 */
    g_free(docfile);
}

extern gchar *logfile_name;

static gboolean delete_log_view(GtkWidget *widget,
                                GdkEvent  *event,
                                gpointer   data )
{
    return FALSE;
}

static void destroy_log_view(GtkWidget *widget,
                             gpointer   data )
{
}

void start_log_view(GtkWidget *w, gpointer data)
{
    gchar line[256];
    FILE *f = fopen(logfile_name, "r");
    if (!f)
        return;

    GtkWindow *window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_title(GTK_WINDOW(window), logfile_name);
    gtk_widget_set_size_request(GTK_WIDGET(window), 500, 560);

    GtkWidget *view = gtk_text_view_new();
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));

    while (fgets(line, sizeof(line), f)) {
        gtk_text_buffer_insert_at_cursor(buffer, line, -1);
    }

    fclose(f);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), view);

    gtk_container_add(GTK_CONTAINER(window), scrolled_window);
    gtk_widget_show_all(GTK_WIDGET(window));

    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (delete_log_view), NULL);
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (destroy_log_view), NULL);

    GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
    gtk_adjustment_set_value(adj, adj->upper - adj->page_size);
    //gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window), adj);
    //gtk_adjustment_value_changed(adj);
}
