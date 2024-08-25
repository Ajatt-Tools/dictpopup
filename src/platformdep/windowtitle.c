#ifdef HAVEX11 // for window title
    #include <X11/Xatom.h>
    #include <X11/Xlib.h>
#endif

#include "platformdep/windowtitle.h"
#include "utils/messages.h"

#include <utils/str.h>

s8 get_windowname(void) {
#ifdef HAVEX11
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        dbg("Can't open X display for retrieving the window title. Are you "
            "using Linux with X11?");
        return (s8){0};
    }

    Window focused_win;
    int revert_to;
    XGetInputFocus(dpy, &focused_win, &revert_to);

    Atom props[] = {XInternAtom(dpy, "_NET_WM_NAME", False), XA_WM_NAME};
    Atom utf8_string = XInternAtom(dpy, "UTF8_STRING", False);
    Atom actual_type;
    int format;
    unsigned long nr_items, bytes_after;
    unsigned char *prop = NULL;

    for (size_t i = 0; i < arrlen(props); i++) {
        if (XGetWindowProperty(dpy, focused_win, props[i], 0, (~0L), False,
                               (props[i] == XA_WM_NAME) ? AnyPropertyType : utf8_string,
                               &actual_type, &format, &nr_items, &bytes_after, &prop) == Success &&
            prop) {
            break;
        }
    }

    XCloseDisplay(dpy);

    s8 ret = s8dup(fromcstr_((char *)prop)); // s.t. cann be freed with free
    XFree(prop);
    return ret;
#else
    // Not implemented
    return (s8){0};
#endif
}