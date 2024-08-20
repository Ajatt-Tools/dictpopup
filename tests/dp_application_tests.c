#include <gtk/gtk.h>
#include <glib.h>
#include "frontends/gtk3popup/dictpopup-application.h"
#include "frontends/gtk3popup/callbacks.h"
#include <ankiconnectc.h>

// Mock functions
static gboolean mock_ac_check_connection(void) {
    return TRUE;
}

static void mock_create_ankicard(s8 lookup_str, s8 sentence, dictentry entry, AnkiConfig cfg) {
    g_assert_cmpstr((char*)lookup_str.s, ==, "test_lookup");
    g_assert_cmpstr((char*)sentence.s, ==, "This is a test sentence.");
    g_assert_cmpstr((char*)entry.definition.s, ==, "This is a test definition.");
}

static void mock_clipboard_set_text(GtkClipboard *clipboard, const gchar *text, gint len) {
    // Do nothing in the mock
}

static gchar* mock_clipboard_wait_for_text(GtkClipboard *clipboard) {
    return g_strdup("This is a test sentence.");
}

static void setup_test_environment(void) {
}

static void test_add_to_anki(void) {
    DpApplication *app = g_object_new(DP_TYPE_APPLICATION,
                                      "application-id", "com.github.Ajatt-Tools.dictpopup",
                                      NULL);

    app->lookup_str = fromcstr_("面白い");

    app->definition_textbuffer = gtk_text_buffer_new(NULL);
    gtk_text_buffer_set_text(app->definition_textbuffer, "This is a test definition.", -1);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(app->definition_textbuffer, &start, &end);
    gtk_text_buffer_select_range(app->definition_textbuffer, &start, &end);

    // app->main_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

    add_to_anki_activated(NULL, NULL, app);

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_emit_by_name(clipboard, "owner-change", NULL);

    while (gtk_events_pending())
        gtk_main_iteration();

    g_object_unref(app);
}

int main(int argc, char *argv[]) {
    setup_test_environment();

    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/dictpopup/add_to_anki", test_add_to_anki);

    return g_test_run();
}