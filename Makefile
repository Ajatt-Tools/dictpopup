.POSIX:
.SUFFIXES:
PREFIX=/usr/local
DESTDIR=
IDIR=include
SDIR=src
LIBDIR=lib
CC=gcc
CFLAGS=-D_POSIX_C_SOURCE=200809L -DNOTIFICATIONS \
       -I$(IDIR) -isystem$(LIBDIR)/lmdb/libraries/liblmdb \
       $(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --cflags libnotify)
DEBUG_CFLAGS=-DDEBUG -g3 -Wall -Wextra -Wdouble-promotion \
	     -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-function \
	     -fsanitize=undefined,address -fsanitize-undefined-trap-on-error
	     #-Wconversion
RELEASE_CFLAGS=-O3 -flto -march=native

LDLIBS=-ffunction-sections -fdata-sections -Wl,--gc-sections \
       -lcurl -lmecab -pthread -lXfixes -lX11 $(shell pkg-config --libs gtk+-3.0) $(shell pkg-config --libs libnotify)

FILES=popup.c util.c platformdep.c deinflector.c settings.c dbreader.c ankiconnectc.c database.c jppron.c pdjson.c
FILES_H=ankiconnectc.h dbreader.h deinflector.h popup.h settings.h util.h platformdep.h database.h jppron.h pdjson.h

LMDB_FILES = $(LIBDIR)/lmdb/libraries/liblmdb/mdb.c $(LIBDIR)/lmdb/libraries/liblmdb/midl.c
SRC=$(addprefix $(SDIR)/,$(FILES)) $(LMDB_FILES)
SRC_H=$(addprefix $(IDIR)/,$(FILES_H))

default: release
release: dictpopup_release dictpopup-create_release
debug: dictpopup_debug dictpopup-create_debug

dictpopup_release: $(SRC) $(SRC_H)
	$(CC) -o dictpopup src/dictpopup.c $(CFLAGS) $(RELEASE_CFLAGS) $(LDLIBS) $(SRC)
dictpopup_debug: $(SRC) $(SRC_H)
	$(CC) -o dictpopup src/dictpopup.c $(CFLAGS) $(DEBUG_CFLAGS) $(LDLIBS) $(SRC)


CFLAGS_CREATE=-I$(IDIR) $(shell pkg-config --cflags glib-2.0)
LDLIBS_CREATE=-ffunction-sections -fdata-sections -Wl,--gc-sections \
	      -lzip $(shell pkg-config --libs glib-2.0)

FILES_CREATE=dbwriter.c pdjson.c util.c settings.c
FILES_H_CREATE=dbwriter.h pdjson.h util.h buf.h settings.h

SRC_CREATE=$(addprefix $(SDIR)/,$(FILES_CREATE))  $(LMDB_FILES)
SRC_H_CREATE=$(addprefix $(IDIR)/,$(FILES_H_CREATE))

dictpopup-create_release: $(SRC_CREATE) $(SRC_H_CREATE)
	$(CC) -o dictpopup-create src/dictpopup_create.c $(CFLAGS_CREATE) $(RELEASE_CFLAGS) $(LDLIBS_CREATE) $(SRC_CREATE)

dictpopup-create_debug: $(SRC_CREATE) $(SRC_H_CREATE)
	$(CC) -o dictpopup-create src/dictpopup_create.c $(CFLAGS_CREATE) $(DEBUG_CFLAGS) $(LDLIBS_CREATE) $(SRC_CREATE) $(SRC_H_CREATE)

CONFIG_DIR=${DESTDIR}${PREFIX}/share/dictpopup

install: copy-config
	mkdir -p ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}$(PREFIX)/share/man/man1
	cp -f dictpopup ${DESTDIR}${PREFIX}/bin
	cp -f dictpopup-create ${DESTDIR}${PREFIX}/bin
	gzip < man/dictpopup.1 > ${DESTDIR}$(PREFIX)/share/man/man1/dictpopup.1.gz
	gzip < man/dictpopup-create.1 > ${DESTDIR}$(PREFIX)/share/man/man1/dictpopup-create.1.gz
	
	mkdir -p $(CONFIG_DIR)
	cp -f config.ini $(CONFIG_DIR)

copy-config:

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup-create
	rm -rf $(CONFIG_DIR)/config.ini

clean:
	rm -f dictpopup dictpopup-create test

test: $(SRC) $(SRC_H)
	gcc -o test $(SDIR)/test.c $(CFLAGS) $(LDLIBS) $(SRC)


.PHONY: clean install uninstall test debug
