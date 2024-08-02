#ifdef GTK3
    #include <gtk/gtk.h>
#endif

#include "platformdep/clipboard.h"
#include <utils/str.h>

// Instead of gtk one could also use: https://github.com/jtanx/libclipboard
s8 get_selection(void) {
#ifdef GTK3
    gtk_init(NULL, NULL);
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    return fromcstr_(gtk_clipboard_wait_for_text(clipboard));
#else
    return S("");
#endif
}

s8 get_clipboard(void) {
#ifdef GTK3
    gtk_init(NULL, NULL);
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    return fromcstr_(gtk_clipboard_wait_for_text(clipboard));
#else
    return S("");
#endif
}