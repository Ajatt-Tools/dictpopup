/* See LICENSE file for license details. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>

#include "util.h"

void
getPointerPosition(int *x, int *y)
{
    static Display *dpy;
    static Window window;
    static int screen;
    if (!(dpy = XOpenDisplay(0)))
	    die("Cannot open display for pointer position.");
    screen = DefaultScreen(dpy);
    window = RootWindow(dpy, screen);

    int di;
    unsigned int du;
    Window dw;
    if (!XQueryPointer(dpy, window, &dw, &dw, x, y, &di, &di, &du))
	  die("Could not query pointer position.");
    XCloseDisplay(dpy);
}

int clipnotify() {
    Display *disp;
    Window root;
    Atom clip;
    XEvent evt;

    disp = XOpenDisplay(NULL);
    if (!disp)
	die("Can't open X display in clipnotify.");

    root = DefaultRootWindow(disp);

    clip = XInternAtom(disp, "CLIPBOARD", False);

    XFixesSelectSelectionInput(disp, root, clip, 
	XFixesSetSelectionOwnerNotifyMask);

    XNextEvent(disp, &evt);

    XCloseDisplay(disp);

    return 0;
}

char*
sselp() {
	Atom clip, utf8, type;
	Display *dpy;
	Window win;
	XEvent ev;
	int fmt;
	long off = 0;
	unsigned char *data;
	unsigned long len, more;

	if(!(dpy = XOpenDisplay(NULL)))
		return "";

	utf8 = XInternAtom(dpy, "UTF8_STRING", False);
	clip = XInternAtom(dpy, "_SSELP_STRING", False);
	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0,
	                          CopyFromParent, CopyFromParent);
	XConvertSelection(dpy, XA_PRIMARY, utf8, clip, win, CurrentTime);

	XNextEvent(dpy, &ev);
	if(ev.type == SelectionNotify && ev.xselection.property != None) {
		XGetWindowProperty(dpy, win, ev.xselection.property, off, BUFSIZ,
				   False, utf8, &type, &fmt, &len, &more, &data);
	}
	XCloseDisplay(dpy);
	return (char*) data;
}
