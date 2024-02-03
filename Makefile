include config.mk

P=dictpopup
SRC = main.c popup.c util.c xlib.c deinflector.c unistr.c readsettings.c dictionary.c dictionarylookup.c kanaconv.c unishox2.c
OBJ = $(SRC:.c=.o)

all: options $(P) dictpopup_create

options:
	@echo $(P) build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

.c.o:
	$(CC) -c $(CFLAGS) $<

$(P): ${OBJ}
	$(CC) ${OBJ} ${LDFLAGS} -o $@ 

dictpopup_create: dictpopup_create.o unishox2.o util.o db_writer.o dictionary.o
	$(CC) $^ ${LDFLAGS} -o $@

clean:
	rm -f $(P) ${OBJ} dictpopup_create dictpopup_create.o unishox2.o util.o db_writer.o

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f $(P) ${DESTDIR}${PREFIX}/bin
	cp -f dictpopup_create ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/$(P)
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup_create

.PHONY: all options clean install uninstall
