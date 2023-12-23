#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>

#include "util.h"

int
clipnotify()
{
	/* Waits for the next clipboard event. */
	Display *disp;
	Window root;
	Atom clip;
	XEvent evt;

	disp = XOpenDisplay(NULL);
	if (!disp)
	{
		fprintf(stderr, "ERROR: Can't open X display for clipnotify. Are you on X11?");
		return 1;
	}

	root = DefaultRootWindow(disp);

	clip = XInternAtom(disp, "CLIPBOARD", False);

	XFixesSelectSelectionInput(disp, root, clip,
				   XFixesSetSelectionOwnerNotifyMask);

	XNextEvent(disp, &evt);

	XCloseDisplay(disp);

	return 0;
}

char*
sselp()
{
	/* Returns a string with the currently selected text */
	/* Return string needs to be freed with free */
	Atom clip, utf8, type;
	Display *dpy;
	Window win;
	XEvent ev;
	int fmt;
	long off = 0;
	unsigned char *data;
	unsigned long len, more;

	if (!(dpy = XOpenDisplay(NULL)))
	{
		fprintf(stderr, "ERROR: Could not open display for the selection. Are you on X11?\n");
		return NULL;
	}

	utf8 = XInternAtom(dpy, "UTF8_STRING", False);
	clip = XInternAtom(dpy, "_SSELP_STRING", False);
	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0,
				  CopyFromParent, CopyFromParent);
	if (!XConvertSelection(dpy, XA_PRIMARY, utf8, clip, win, CurrentTime))
	{
		fprintf(stderr, "ERROR: Could not convert selection.\n");
		XCloseDisplay(dpy);
		return NULL;
	}

	XNextEvent(dpy, &ev);
	if (ev.type == SelectionNotify && ev.xselection.property != None)
	{
		XGetWindowProperty(dpy, win, ev.xselection.property, off, BUFSIZ,
				   False, utf8, &type, &fmt, &len, &more, &data);
	}
	else
	{
		XCloseDisplay(dpy);
		return NULL;
	}

	XCloseDisplay(dpy);

	/* char *data2 = (char *)data; */
	/* printf("data: %s, strlen: %li, len: %li", data2, strlen(data2), len); */

	char *selection = !data ? NULL : strdup((char *)data);
	XFree(data);
	return selection;
}

char*
getwindowname()
{
	/* Returns the window title of the currently active window */
	/* return string needs to be freed */
	Display *dpy;
	Window focused;
	XTextProperty text_prop;
	int revert_to;

	if (!(dpy = XOpenDisplay(NULL)))
	{
		fprintf(stderr, "ERROR: Could not open display for the window name. Are you on X11?\n");
		return NULL;
	}

	XGetInputFocus(dpy, &focused, &revert_to);

	if (!XGetWMName(dpy, focused, &text_prop))
	{
		fprintf(stderr, "ERROR: Could not obtain window name. \n");
		return NULL;
	}

	char *wname = strdup((char *)text_prop.value);
	XFree(text_prop.value);
	XCloseDisplay(dpy);
	return wname;
}
