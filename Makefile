CFLAGS = -Wall -Wextra -pedantic -lX11 -lXft -I/usr/include/freetype2

PREFIX ?= /usr/local
CC ?= cc

all: herbe

config.h: config.def.h
	cp config.def.h config.h

herbe: popup.c config.h
	$(CC) popup.c $(CFLAGS) -o popup

install: herbe
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f herbe ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/herbe

clean:
	rm -f herbe

.PHONY: all install uninstall clean
