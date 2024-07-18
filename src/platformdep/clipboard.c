#include <gtk/gtk.h>

#include "platformdep/clipboard.h"
#include "utils/util.h"

#include <utils/str.h>

// Instead of gtk one could also use: https://github.com/jtanx/libclipboard
s8 get_selection(void) {
    gtk_init(NULL, NULL);
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    return fromcstr_(gtk_clipboard_wait_for_text(clipboard));
}

char *get_clipboard(void) {
    gtk_init(NULL, NULL);
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    return gtk_clipboard_wait_for_text(clipboard);
}

static void cb_changed_callback(GtkClipboard *clipboard, gpointer user_data) {
    gtk_main_quit();
}

static void wait_cb_change(void) {
    gtk_init(NULL, NULL);
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(cb_changed_callback), NULL);
    gtk_main();
}

/*
 * Wait until clipboard changes and return new clipboard contents.
 * Can return NULL.
 */
char *get_next_clipboard(void) {
    wait_cb_change();
    return get_clipboard();
}