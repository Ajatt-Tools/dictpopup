# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

PKGCONFIG = $(shell which pkg-config)
GTK3CFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-3.0)
GTK3LIBS = $(shell $(PKGCONFIG) --libs gtk+-3.0)

# includes and libs
INCS = -I$(X11INC)
LIBS = -L$(X11LIB) -lX11 $(FREETYPELIBS) -lcurl -lXfixes -lankiconnectc $(GTK3LIBS) -lmecab
# LIBS = -L$(X11LIB) -lX11 $(FREETYPELIBS) -lcurl -lXfixes $(GTK3LIBS) $(NOTIFYLIBS)

# flags
# CFLAGS   = -std=c17 -O3 -g -pedantic -Wall -fanalyzer $(INCS) $(GTK3CFLAGS) -pthread
CFLAGS   = -std=c17 -O3 -g -pedantic -Wall -Werror $(INCS) $(GTK3CFLAGS) -pthread
LDFLAGS  = $(LIBS) -g -lpthread -lcdb -llmdb

# compiler and linker
CC = cc
