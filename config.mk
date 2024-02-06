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
LIBS = -L$(X11LIB) -lX11 $(FREETYPELIBS) -lcurl -lXfixes $(GTK3LIBS) -lmecab -llmdb -pthread
# LIBS = -L$(X11LIB) -lX11 $(FREETYPELIBS) -lcurl -lXfixes $(GTK3LIBS) $(NOTIFYLIBS)

# flags
#debug flags
# CFLAGS = -g3 -Wall -Wextra -Wno-unused-parameter -Wdouble-promotion -fsanitize=undefined,unreachable -fsanitize-trap $(GTK3CFLAGS) 
#relase build flags
CFLAGS   = -O3 $(GTK3CFLAGS) 
LDFLAGS = $(LIBS)

# compiler and linker
CC = cc
