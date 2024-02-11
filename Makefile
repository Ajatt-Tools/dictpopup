.POSIX:
.SUFFIXES:
IDIR=include
SDIR=src
CC=gcc
CFLAGS=-I$(IDIR) $(shell pkg-config --cflags gtk+-3.0) -Wall
DEBUG_FLAGS=-g3 -Wextra -Wdouble-promotion -Wconversion -Wno-sign-conversion -Wno-unused-parameter -fsanitize=undefined,address -fsanitize-undefined-trap-on-error
RELEASE_FLAGS=-O3 -flto
LIBS = $(shell pkg-config --libs gtk+-3.0) -lcurl -lmecab -llmdb -pthread -lXfixes -lX11
PREFIX = /usr/local

FILES = dictpopup.c popup.c util.c xlib.c deinflector.c settings.c dbreader.c unishox2.c ankiconnectc.c
FILES_H = ankiconnectc.h dbreader.h deinflector.h popup.h settings.h util.h xlib.h unishox2.h
SRC = $(addprefix $(SDIR)/,$(FILES))
SRC_H = $(addprefix $(IDIR)/,$(FILES_H))

FILES_CREATE = dictpopup_create.c unishox2.c util.c dbwriter.c
FILES_CREATE_H = dbwriter.h unishox2.h util.h
SRC_CREATE = $(addprefix $(SDIR)/,$(FILES_CREATE))
SRC_CREATE_H = $(addprefix $(IDIR)/,$(FILES_CREATE_H))

default: release

release: dictpopup_release dictpopup-create_release
dictpopup_release: $(SRC) $(SRC_H)
	$(CC) -o dictpopup $(SRC) $(CFLAGS) $(RELEASE_FLAGS) $(LIBS)
dictpopup-create_release: $(SRC_CREATE) $(SRC_CREATE_H)
	$(CC) -o dictpopup-create $(SRC_CREATE) $(CFLAGS) $(RELEASE_FLAGS) $(LIBS)

debug: dictpopup_debug dictpopup-create_debug
dictpopup_debug: $(SRC) $(SRC_H)
	$(CC) -o dictpopup $(SRC) $(CFLAGS) $(DEBUG_FLAGS) $(LIBS)
dictpopup-create_debug: $(SRC_CREATE) $(SRC_CREATE_H)
	$(CC) -o dictpopup-create $(SRC_CREATE) $(SRC_CREATE_H) $(CFLAGS) $(DEBUG_FLAGS) $(LIBS)

install:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	cp -f dictpopup ${DESTDIR}${PREFIX}/bin
	cp -f dictpopup-create ${DESTDIR}${PREFIX}/bin
	gzip < dictpopup.1 > $(DESTDIR)$(PREFIX)/share/man/man1/dictpopup.1.gz
	gzip < dictpopup-create.1 > $(DESTDIR)$(PREFIX)/share/man/man1/dictpopup-create.1.gz

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup-create

clean:
	rm -f dictpopup dictpopup-create

.PHONY: clean install uninstall
