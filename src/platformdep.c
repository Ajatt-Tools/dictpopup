#include "util.h"
#include <gtk/gtk.h>
#include <libnotify/notify.h>

#ifdef HAVEX11 // for window title
    #include <X11/Xatom.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/extensions/Xfixes.h>
#endif

#include "messages.h"

void notify(const char *title, _Bool urgent, char const *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    g_autofree char *txt = g_strdup_vprintf(fmt, argp);
    va_end(argp);
    if (!txt)
        return;

    notify_init("dictpopup");
    NotifyNotification *n = notify_notification_new(title, txt, NULL);
    notify_notification_set_urgency(n, urgent ? NOTIFY_URGENCY_CRITICAL : NOTIFY_URGENCY_NORMAL);
    notify_notification_set_timeout(n, 10000);

    GError *err = NULL;
    if (!notify_notification_show(n, &err)) {
        fprintf(stderr, "Failed to send notification: %s\n", err->message);
        g_error_free(err);
    }
    g_object_unref(G_OBJECT(n));
}

// Instead of gtk one could also use: https://github.com/jtanx/libclipboard
s8 get_selection(void) {
    gtk_init(NULL, NULL);
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    return fromcstr_(gtk_clipboard_wait_for_text(clipboard));
}

/* -- Sentence -- */
static s8 get_clipboard(void) {
    gtk_init(NULL, NULL);
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    return fromcstr_(gtk_clipboard_wait_for_text(clipboard));
}

static void cb_changed(GtkClipboard *clipboard, gpointer user_data) {
    gtk_main_quit();
}

static void wait_cb_change(void) {
    gtk_init(NULL, NULL);
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(cb_changed), NULL);
    gtk_main();
}

/*
 * Wait until clipboard changes and return new clipboard contents.
 * Can return NULL.
 */
s8 get_sentence(void) {
    wait_cb_change();
    return get_clipboard();
}
/* -------------- */

s8 get_windowname(void) {
#ifdef HAVEX11
    XTextProperty text_prop;

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        dbg("Can't open X display for retrieving the window title. Are you "
            "using Linux with X11?");
        return (s8){0};
    }

    Window focused_win;
    int revert_to;
    XGetInputFocus(dpy, &focused_win, &revert_to);

    /* --------------- */
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
            return fromcstr_((char *)prop);
        }
    }
    return (s8){0};
    /* --------------- */
    /* if (!XGetWMName(dpy, focused_win, &text_prop)) */
    /* { */
    /* 	dbg("Could not obtain window name."); */
    /* 	return (s8){0}; */
    /* } */
    /* XCloseDisplay(dpy); */

    return fromcstr_((char *)text_prop.value);
#else
    // Not implemented
    return (s8){0};
#endif
}

void play_audio(s8 filepath) {
    _drop_(frees8) s8 cmd =
        concat(S("ffplay -nodisp -nostats -hide_banner -autoexit '"), filepath, S("'"));

    GError *error = NULL;
    g_spawn_command_line_sync((char *)cmd.s, NULL, NULL, NULL, &error);
    if (error) {
        err("Failed to play file %s: %s", filepath.s, error->message);
        g_error_free(error);
    }
}
