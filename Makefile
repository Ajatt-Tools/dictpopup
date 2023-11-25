include config.mk

SRC = main.c popup.c util.c xlib.c
OBJ = $(SRC:.c=.o)

all: options dictpopup

options:
	@echo popup build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

.c.o:
	$(CC) -O3 -c $(CFLAGS) $<

$(OBJ): config.h config.mk

config.h:
	cp config.def.h $@

dictpopup: ${OBJ}
	$(CC) -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f dictpopup ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dictpopup ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup

.PHONY: all options clean install uninstall
