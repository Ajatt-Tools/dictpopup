#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>

#include <glib.h>

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
		fprintf(stderr, "ERROR: Can't open X display for clipnotify. Are you using Linux with X11?");
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
		fprintf(stderr, "ERROR: Can't open X display for retrieving the window title. Are you using Linux with X11?");
		return NULL;
	}

	XGetInputFocus(dpy, &focused, &revert_to);

	if (!XGetWMName(dpy, focused, &text_prop))
	{
		fprintf(stderr, "ERROR: Could not obtain window name. \n");
		return NULL;
	}

	char *wname = g_strdup((char *)text_prop.value);
	XFree(text_prop.value);
	XCloseDisplay(dpy);
	return wname;
}
