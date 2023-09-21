include config.mk

SRC = popup.c format_output.c deinflector.c
OBJ = $(SRC:.c=.o)

all: options dictpopup

options:
	@echo popup build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

config.h:
	cp config.def.h config.h

.c.o:
	$(CC) -c $(CFLAGS) $<

popup.o: config.h

$(OBJ): config.h config.mk

dictpopup: $(OBJ)
	$(CC) -o format_output format_output.o
	$(CC) -o deinflector deinflector.o
	$(CC) -o popup popup.o $(LDFLAGS)

clean:
	rm -f popup $(OBJ) format_output deinflector

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f popup ${DESTDIR}${PREFIX}/bin
	cp -f dictpopup ${DESTDIR}${PREFIX}/bin
	cp -f format_output ${DESTDIR}${PREFIX}/bin
	cp -f deinflector ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/popup
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup
	rm -f ${DESTDIR}${PREFIX}/bin/format_output
	rm -f ${DESTDIR}${PREFIX}/bin/deinflector


.PHONY: all options clean install uninstall
