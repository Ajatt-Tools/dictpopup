include config.mk

SRC = popup.c
OBJ = $(SRC:.c=.o)

all: options popup

options:
	@echo popup build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

.c.o:
	$(CC) -c $(CFLAGS) $<

popup: popup.o
	$(CC) -o $@ popup.o $(LDFLAGS)

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f popup ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/popup

clean:
	rm -f popup $(OBJ)

.PHONY: all install uninstall clean
