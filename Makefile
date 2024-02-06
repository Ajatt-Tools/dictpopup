include config.mk

P=dictpopup
SRC = main.c popup.c util.c xlib.c deinflector.c readsettings.c dictionary.c dictionarylookup.c kanaconv.c unishox2.c ankiconnectc.c
SRC_CREATE = dictpopup_create.c unishox2.c util.c db_writer.c dictionary.c

all: options $(P) dictpopup_create

options:
	@echo $(P) build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

${P}:
	$(CC) ${SRC} ${CFLAGS} ${LDFLAGS} -o $@ 

${P}_create:
	$(CC) ${SRC_CREATE} ${CFLAGS} ${LDFLAGS} -o $@ 

clean:
	rm -f $(P) dictpopup_create

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f $(P) ${DESTDIR}${PREFIX}/bin
	cp -f dictpopup_create ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/$(P)
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup_create

.PHONY: all options clean install uninstall
