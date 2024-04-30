.POSIX:
.SUFFIXES:
CC=gcc
PREFIX=/usr/local
MANPREFIX = ${PREFIX}/share/man

IDIR=include
SDIR=src
LDIR=lib

CPPFLAGS=-D_POSIX_C_SOURCE=200809L -DNOTIFICATIONS -I$(IDIR)
LDLIBS=-ffunction-sections -fdata-sections -Wl,--gc-sections \
       -lcurl -lmecab -pthread $(shell pkg-config --libs gtk+-3.0) \
       $(shell pkg-config --libs libnotify) -llmdb -lglib-2.0

O_HAVEX11 := 1  # X11 integration
ifeq ($(strip $(O_HAVEX11)),1)
	CPPFLAGS += -DHAVEX11
	LDLIBS += -lXfixes -lX11
endif

CFLAGS= ${CPPFLAGS} $(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --cflags libnotify)
DEBUG_CFLAGS=-DDEBUG -g3 -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wdouble-promotion -Wshadow \
	     -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-function \
	     -fsanitize=undefined,address -fsanitize-undefined-trap-on-error
RELEASE_CFLAGS=-O3 -flto -march=native

FILES=gtk3popup.c util.c platformdep.c deinflector.c settings.c dbreader.c ankiconnectc.c database.c jppron.c pdjson.c
FILES_H=ankiconnectc.h dbreader.h deinflector.h gtk3popup.h settings.h util.h platformdep.h database.h jppron.h pdjson.h
SRC=$(addprefix $(SDIR)/,$(FILES))
SRC_H=$(addprefix $(IDIR)/,$(FILES_H))


CFLAGS_CREATE=-I$(IDIR) -isystem$(LIBDIR)/lmdb/libraries/liblmdb -D_POSIX_C_SOURCE=200809L $(shell pkg-config --cflags glib-2.0)
LDLIBS_CREATE=-ffunction-sections -fdata-sections -Wl,--gc-sections \
	      -lzip $(shell pkg-config --libs glib-2.0) -llmdb

FILES_CREATE=dbwriter.c pdjson.c util.c settings.c
FILES_H_CREATE=dbwriter.h pdjson.h util.h buf.h settings.h

SRC_CREATE=$(addprefix $(SDIR)/,$(FILES_CREATE))  $(LMDB_FILES)
SRC_H_CREATE=$(addprefix $(IDIR)/,$(FILES_H_CREATE))

default: dictpopup dictpopup-create
debug: dictpopup-debug dictpopup-create-debug

dictpopup: $(SRC) $(SRC_H)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -o $@ src/dictpopup.c $(SRC) $(LDLIBS)
dictpopup-debug: $(SRC) $(SRC_H)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $@ src/dictpopup.c $(SRC) $(LDLIBS)

dictpopup-create: $(SRC_CREATE) $(SRC_H_CREATE)
	$(CC) $(CFLAGS_CREATE) $(RELEASE_CFLAGS) -o $@ src/dictpopup_create.c $(SRC_CREATE) $(LDLIBS_CREATE)
dictpopup-create-debug: $(SRC_CREATE) $(SRC_H_CREATE)
	$(CC) $(CFLAGS_CREATE) $(DEBUG_CFLAGS) -o $@ src/dictpopup_create.c $(SRC_CREATE) $(LDLIBS_CREATE)

release:
	version=$$(git describe); prefix=dictpopup-$${version#v}; \
	  git archive --prefix=$$prefix/ HEAD | gzip -9 >$$prefix.tar.gz

CONFIG_DIR=${DESTDIR}${PREFIX}/share/dictpopup

install: default
	mkdir -p ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}$(PREFIX)/share/man/man1

	cp -f dictpopup ${DESTDIR}${PREFIX}/bin
	cp -f dictpopup-create ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dictpopup
	chmod 755 ${DESTDIR}${PREFIX}/bin/dictpopup-create

	gzip < man/dictpopup.1 > ${DESTDIR}${MANPREFIX}/man1/dictpopup.1.gz
	gzip < man/dictpopup-create.1 > ${DESTDIR}${MANPREFIX}/man1/dictpopup-create.1.gz
	
	mkdir -p $(CONFIG_DIR)
	cp -f config.ini $(CONFIG_DIR)

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup \
	      ${DESTDIR}${PREFIX}/bin/dictpopup-create \
	      ${DESTDIR}${MANPREFIX}/man1/dictpopup.1.gz \
	      ${DESTDIR}${MANPREFIX}/man1/dictpopup-create.1.gz \
	      $(CONFIG_DIR)/config.ini

clean:
	rm -f dictpopup dictpopup-create test

tests: $(SRC) $(SRC_H)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $@ $(SDIR)/test.c $(SRC) $(LDLIBS)

check test: tests
	./tests

.PHONY: all clean install uninstall tests debug
