# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2
# OpenBSD (uncomment)
#FREETYPEINC = $(X11INC)/freetype2
#MANPREFIX = ${PREFIX}/man

PKGCONFIG = $(shell which pkg-config)
GTK3CFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-3.0)
GTK3LIBS = $(shell $(PKGCONFIG) --libs gtk+-3.0)

NOTIFYCFLAGS = $(shell $(PKGCONFIG) --cflags libnotify)
NOTIFYLIBS = $(shell $(PKGCONFIG) --libs libnotify)

# includes and libs
INCS = -I$(X11INC) -I$(FREETYPEINC)
LIBS = -L$(X11LIB) -lX11 $(XINERAMALIBS) $(FREETYPELIBS) -lcurl -lXfixes $(GTK3LIBS) $(NOTIFYLIBS)


# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\" $(XINERAMAFLAGS)
CFLAGS   = -std=c99 -pedantic -Wall -Os $(INCS) $(CPPFLAGS) $(GTK3CFLAGS) $(NOTIFYCFLAGS)
LDFLAGS  = $(LIBS) 

# compiler and linker
CC = cc
