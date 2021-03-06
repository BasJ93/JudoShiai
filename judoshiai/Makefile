#PROF=-pg
PROF=

include ../common/Makefile.inc

OBJDIR := $(JS_BUILD_DIR)/$(LASTPART)/$(OBJDIR)

all: $(OBJDIR) $(OBJDIR)/judoshiai$(SUFF)

$(OBJDIR):
	mkdir -p $(OBJDIR)

OBJS += $(OBJDIR)/main.o $(OBJDIR)/competitors.o $(OBJDIR)/sheets.o $(OBJDIR)/set-one-judoka.o \
	$(OBJDIR)/sqlite3-patched.o $(OBJDIR)/popup-menu.o $(OBJDIR)/util.o $(OBJDIR)/db.o \
	$(OBJDIR)/categories.o $(OBJDIR)/log.o $(OBJDIR)/matches.o \
	$(OBJDIR)/names.o $(OBJDIR)/comm.o $(OBJDIR)/ip.o $(OBJDIR)/protocol.o \
	$(OBJDIR)/set-categories.o $(OBJDIR)/category-graph.o \
	$(OBJDIR)/completion.o $(OBJDIR)/another-shiai.o $(OBJDIR)/avl.o $(OBJDIR)/httpp.o \
	$(OBJDIR)/httpd.o $(OBJDIR)/import.o $(OBJDIR)/set-password.o \
	$(OBJDIR)/trees.o $(OBJDIR)/match-graph.o $(OBJDIR)/search.o $(OBJDIR)/menus.o \
	$(OBJDIR)/menuacts.o $(OBJDIR)/sql-dialog.o $(OBJDIR)/drawing.o $(OBJDIR)/print.o \
	$(OBJDIR)/print-texts.o $(OBJDIR)/db-competitors.o $(OBJDIR)/db-categories.o \
	$(OBJDIR)/db-matches.o $(OBJDIR)/db-table.o $(OBJDIR)/write-results.o \
	$(OBJDIR)/match-data.o $(OBJDIR)/serial.o \
	$(OBJDIR)/basic.tab.o $(OBJDIR)/lex.yy.o $(OBJDIR)/medal-matches.o $(OBJDIR)/db-validate.o \
	$(OBJDIR)/svg.o $(OBJDIR)/properties.o $(OBJDIR)/ftp.o $(OBJDIR)/language.o \
	$(OBJDIR)/custom-category-parser.o $(OBJDIR)/custom-category.o \
	$(OBJDIR)/custom-category-svg-template.o $(OBJDIR)/common.o $(OBJDIR)/sha1.o \
	$(OBJDIR)/websock.o $(OBJDIR)/websock-protocol.o  $(OBJDIR)/cJSON.o 

CFLAGS += -DAPPLICATION="\"judoshiai\""
CFLAGS += -DAPP_NUM=1
CFLAGS += -I../3rdparty/httpp -I../3rdparty/sqlite3 -I../3rdparty/cJSON

### Windows executable ###
ifeq ($(TGT),WIN32)
OBJS += judoshiai-icon.res

judoshiai-icon.res: judoshiai-icon.rc
	$(WINDRES) -O coff judoshiai-icon.rc judoshiai-icon.res

judoshiai-icon.rc: ../etc/judoshiai.ico
	echo "APPICONS ICON \"../etc/judoshiai.ico\"" >judoshiai-icon.rc

### Linux executable ###
else
PROF += -rdynamic
endif

$(OBJDIR)/judoshiai$(SUFF): $(OBJS)
	$(LD) $(PROF) -g -Wl,-rpath -Wl,bin $(OBJS) -o $(OBJDIR)/judoshiai$(SUFF) $(LIBS)

$(OBJDIR)/avl.o: ../common/avl.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/ip.o: ../common/ip.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/protocol.o: ../common/protocol.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/binreloc.o: ../common/binreloc.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/serial.o: ../common/serial.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/language.o: ../common/language.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/sha1.o: ../common/sha1.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c judoshiai.h ../common/comm.h
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/common.o: ../common/common.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/websock-protocol.o: ../common/websock-protocol.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/cJSON.o: ../3rdparty/cJSON/cJSON.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/httpp.o: ../3rdparty/httpp/httpp.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

sqlite3-patched.c: ../3rdparty/sqlite3/sqlite3.c sqlite3.patch
	cp ../3rdparty/sqlite3/sqlite3.c sqlite3-patched.c
	patch -p2 sqlite3-patched.c <sqlite3.patch

lex.yy.c: basic.l basic.tab.h
	flex basic.l

basic.tab.c basic.tab.h: basic.y
	bison -t -v -d basic.y

clean:
	rm -f $(OBJDIR)/* *.res *.rc *~ lex.yy.c basic.tab.c basic.tab.h
