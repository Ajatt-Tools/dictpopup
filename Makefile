.POSIX:
.SUFFIXES:
CC=gcc
PREFIX=/usr/local
MANPREFIX = ${PREFIX}/share/man

IDIR=include
SDIR=src
LDIR=lib

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -I$(IDIR)
CFLAGS := -Werror $(shell pkg-config --cflags gtk+-3.0) $(CFLAGS)
DEBUG_CFLAGS = -DDEBUG \
	     -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wdouble-promotion -Wshadow \
	     -Wno-unused-parameter -Wno-sign-conversion -Wpointer-arith \
	     -Wmissing-prototypes -Wstrict-prototypes -Wstrict-overflow -Wcast-align \
	     -fsanitize=address,undefined -fsanitize-undefined-trap-on-error -fstack-protector-strong \
	     -O0 -ggdb
RELEASE_CFLAGS = -Ofast -flto -march=native
NOTIF_CFLAGS := -DNOTIFICATIONS $(shell pkg-config --cflags libnotify) $(shell pkg-config --libs libnotify)
NOTIF_LIBS := $(shell pkg-config --libs libnotify)
LDLIBS +=-lcurl -lmecab $(shell pkg-config --libs gtk+-3.0) -llmdb -lzip -lyyjson

O_HAVEX11 := 1  # X11 integration
ifeq ($(strip $(O_HAVEX11)),1)
	CPPFLAGS += -DHAVEX11
	LDLIBS += -lXfixes -lX11
endif


FILES = dictpopup.c util.c platformdep.c deinflector.c settings.c db.c ankiconnectc.c database.c jppron.c pdjson.c
FILES_H = ankiconnectc.h db.h deinflector.h settings.h util.h platformdep.h database.h jppron.h pdjson.h
SRC = $(addprefix $(SDIR)/,$(FILES))
SRC_H = $(addprefix $(IDIR)/,$(FILES_H))

FILES_CREATE = db.c pdjson.c util.c platformdep.c yomichan_parser.c
FILES_H_CREATE = db.h pdjson.h util.h buf.h platformdep.h
SRC_CREATE = $(addprefix $(SDIR)/,$(FILES_CREATE))  $(LMDB_FILES)
SRC_H_CREATE = $(addprefix $(IDIR)/,$(FILES_H_CREATE))

bins := dictpopup dictpopup-create

all: $(bins)
all: CFLAGS+=$(RELEASE_CFLAGS)

debug: $(bins)
debug: CFLAGS+=$(DEBUG_CFLAGS)

dictpopup: $(SRC) $(SRC_H) $(SDIR)/frontends/gtk3popup.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(NOTIF_CFLAGS) -o $@ $(SDIR)/frontends/gtk3popup.c $(SRC) $(LDLIBS) $(NOTIF_LIBS)

dictpopup-create: $(SRC_CREATE) $(SRC_H_CREATE) $(SDIR)/dictpopup_create.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SDIR)/dictpopup_create.c $(SRC_CREATE) $(LDLIBS)

cli: $(SRC) $(SRC_H) $(SDIR)/frontends/cli.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) $(CPPFLAGS) -o $@ $(SDIR)/frontends/cli.c $(SRC) $(LDLIBS)

deinflector: $(SRC) $(SRC_H) $(SDIR)/deinflector.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) $(CPPFLAGS) -DDEINFLECTOR_MAIN -o $@ $(SDIR)/deinflector.c $(SDIR)/util.c $(LDLIBS)

release:
	version=$$(git describe); prefix=dictpopup-$${version#v}; \
	  git archive --prefix=$$prefix/ HEAD | gzip -9 >$$prefix.tar.gz

CONFIG_DIR=${DESTDIR}${PREFIX}/share/dictpopup

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}$(PREFIX)/share/man/man1

	cp -f dictpopup ${DESTDIR}${PREFIX}/bin
	cp -f dictpopup-create ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dictpopup
	chmod 755 ${DESTDIR}${PREFIX}/bin/dictpopup-create

	gzip < man1/dictpopup.1 > ${DESTDIR}${MANPREFIX}/man1/dictpopup.1.gz
	gzip < man1/dictpopup-create.1 > ${DESTDIR}${MANPREFIX}/man1/dictpopup-create.1.gz
	
	mkdir -p $(CONFIG_DIR)
	cp -f config.ini $(CONFIG_DIR)

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dictpopup \
	      ${DESTDIR}${PREFIX}/bin/dictpopup-create \
	      ${DESTDIR}${MANPREFIX}/man1/dictpopup.1.gz \
	      ${DESTDIR}${MANPREFIX}/man1/dictpopup-create.1.gz \
	      $(CONFIG_DIR)/config.ini

clean:
	rm -f dictpopup dictpopup-create cli deinflector


ALL_C_SOURCES := $(wildcard src/*.c)
ALL_H_SOURCES := $(wildcard include/*.h)

c_analyse_targets := $(ALL_C_SOURCES:%=%-analyse)
c_analyse_targets := $(filter-out src/yyjson.c-analyse, $(c_analyse_targets))

h_analyse_targets := $(ALL_H_SOURCES:%=%-analyse)
h_analyse_targets := $(filter-out include/yyjson.h-analyse, $(h_analyse_targets))

analyse: CFLAGS+=$(DEBUG_CFLAGS)
analyse: $(c_analyse_targets) $(h_analyse_targets)

$(c_analyse_targets): %-analyse:
	echo "$(c_analyse_targets)"
	# -W options here are not clang compatible, so out of generic CFLAGS
	gcc $< -o /dev/null -c \
		-std=gnu99 -Ofast -fwhole-program -Wall -Wextra \
		-Wlogical-op -Wduplicated-cond \
		-fanalyzer $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS)
	clang $< -o /dev/null -c -std=gnu99 -Ofast -Weverything \
		-Wno-documentation-unknown-command \
		-Wno-language-extension-token \
		-Wno-disabled-macro-expansion \
		-Wno-padded \
		-Wno-covered-switch-default \
		-Wno-gnu-zero-variadic-macro-arguments \
		-Wno-declaration-after-statement \
		-Wno-cast-qual \
		-Wno-unused-command-line-argument \
		$(extra_clang_flags) \
		$(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS)
	$(MAKE) $*-shared-analyse

$(h_analyse_targets): %-analyse:
	$(MAKE) $*-shared-analyse

%-shared-analyse: %
	# cppcheck is a bit dim about unused functions/variables, leave that to
	# clang/GCC
	cppcheck $< -I$(IDIR) --library=gtk.cfg --library=libcurl.cfg \
		--std=c99 --quiet --inline-suppr --force --enable=all \
		--suppress=missingIncludeSystem --suppress=unmatchedSuppression \
		--suppress=unreadVariable --suppress=constParameterCallback \
		--suppress=constVariablePointer --suppress=constParameterPointer \
		--suppress=unusedFunction --suppress=*:include/yyjson.h \
		--max-ctu-depth=32 --error-exitcode=1
	# clang-analyzer-unix.Malloc does not understand _drop_()
	clang-tidy $< --quiet -checks=-clang-analyzer-unix.Malloc -- -std=gnu99 -I$(IDIR) $(shell pkg-config --cflags gtk+-3.0)
	clang-format --dry-run --Werror $<	


.PHONY: all clean install uninstall debug analyse
