.POSIX:
.SUFFIXES:
PREFIX=/usr/local
IDIR=include
SDIR=src
LIBDIR=lib
CC=gcc
CFLAGS=-I$(IDIR) -isystem $(LIBDIR) -Wall -D_POSIX_C_SOURCE=200809L -std=c17 \
       -Wno-unused-function $(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --cflags libnotify)
DEBUG_FLAGS= -DDEBUG -fsanitize=undefined,address -fsanitize-undefined-trap-on-error \
	     -g3 -Wextra -Wdouble-promotion -Wno-unused-parameter \
	     -pedantic -Wno-sign-conversion
# -Wconversion 
RELEASE_FLAGS=-O3 -flto
LDLIBS = $(shell pkg-config --libs gtk+-3.0) -lcurl -lmecab -pthread -lXfixes -lX11 $(shell pkg-config --libs libnotify)
LDLIBS_CREATE = -lzip $(shell pkg-config --libs gtk+-3.0) $(shell pkg-config --libs libnotify)

FILES = popup.c util.c platformdep.c deinflector.c settings.c dbreader.c ankiconnectc.c database.c jppron.c pdjson.c
FILES_H = ankiconnectc.h dbreader.h deinflector.h popup.h settings.h util.h platformdep.h database.h jppron.h pdjson.h

FILES_CREATE = util.c dbwriter.c
FILES_CREATE_H = dbwriter.h util.h

LDLIBS += -llmdb
LDLIBS_CREATE += -llmdb
# LIB_SRC = lib/mdb.c lib/midl.c
# LIB_SRC_H = lib/lmdb.h lib/midl.h

SRC = $(addprefix $(SDIR)/,$(FILES))
SRC_H = $(addprefix $(IDIR)/,$(FILES_H))

CREATE_SRC = $(addprefix $(SDIR)/,$(FILES_CREATE))
CREATE_SRC_H = $(addprefix $(IDIR)/,$(FILES_CREATE_H))

default: release

release: dictpopup_release dictpopup-create_release
dictpopup_release: $(SRC) $(SRC_H) $(LIB_SRC) $(LIB_SRC_H)
	$(CC) -o dictpopup src/dictpopup.c $(CFLAGS) $(RELEASE_FLAGS) $(LDLIBS) $(SRC) $(LIB_SRC)
dictpopup-create_release: $(CREATE_SRC) $(CREATE_SRC_H) $(LIB_SRC) $(LIB_SRC_H)
	$(CC) -o dictpopup-create src/dictpopup_create.c $(CFLAGS) $(RELEASE_FLAGS) $(LDLIBS_CREATE) $(CREATE_SRC) $(LIB_SRC)

debug: dictpopup_debug dictpopup-create_debug
dictpopup_debug: $(SRC) $(SRC_H) $(LIB_SRC) $(LIB_SRC_H)
	$(CC) -o dictpopup src/dictpopup.c $(CFLAGS) $(DEBUG_FLAGS) $(LDLIBS) $(SRC) $(LIB_SRC)
dictpopup-create_debug: $(CREATE_SRC) $(CREATE_SRC_H) $(LIB_SRC) $(LIB_SRC_H)
	$(CC) -o dictpopup-create src/dictpopup_create.c $(CFLAGS) $(DEBUG_FLAGS) $(LDLIBS_CREATE) $(CREATE_SRC) $(CREATE_SRC_H) $(LIB_SRC)

install:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	cp -f dictpopup ${DESTDIR}${PREFIX}/bin
	cp -f dictpopup-create ${DESTDIR}${PREFIX}/bin
	gzip < man/dictpopup.1 > $(DESTDIR)$(PREFIX)/share/man/man1/dictpopup.1.gz
	gzip < man/dictpopup-create.1 > $(DESTDIR)$(PREFIX)/share/man/man1/dictpopup-create.1.gz

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup-create

clean:
	rm -f dictpopup dictpopup-create test

test: $(SRC) $(SRC_H)
	gcc -o test $(SDIR)/test.c $(CFLAGS) $(LDLIBS) $(SRC)


.PHONY: clean install uninstall
