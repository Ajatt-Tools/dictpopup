include config.mk

P=dictpopup
SRC = main.c popup.c util.c xlib.c deinflector.c unistr.c readsettings.c dictionary.c dictionarylookup.c kanaconv.c
OBJ = $(SRC:.c=.o)

all: options $(P)

options:
	@echo $(P) build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

.c.o:
	$(CC) -c $(CFLAGS) $<

$(P): ${OBJ}
	$(CC) ${OBJ} ${LDFLAGS} -o $@ 

clean:
	rm -f $(P) ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f $(P) ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/$(P)

.PHONY: all options clean install uninstall
