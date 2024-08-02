#include "dictpopup-application.h"
#include "gtk/gtk.h"

#include "callbacks.h"
#include <ankiconnectc.h>
#include <dictpopup.h>
#include <utils/messages.h>

/* --------------- START CALLBACKS ------------------ */
void on_settings_button_clicked(GtkButton *button, gpointer user_data) {
    DpApplication *dp = (DpApplication *)user_data;

    if (dp->settings_window == NULL) {
        GtkBuilder *builder =
            gtk_builder_new_from_resource("/com/github/Ajatt-Tools/dictpopup/settings-window.ui");
        dp->settings_window = GTK_WIDGET(gtk_builder_get_object(builder, "settings_window"));

        if (dp->settings_window == NULL) {
            g_print("Error: Failed to initialize settings window\n");
            g_object_unref(builder);
            return;
        }

        gtk_window_set_transient_for(GTK_WINDOW(dp->settings_window), GTK_WINDOW(dp->main_window));
        g_signal_connect(dp->settings_window, "delete-event", G_CALLBACK(gtk_widget_destroy), NULL);

        g_object_unref(builder);
    }

    gtk_widget_show_all(dp->settings_window);
}

/* -------------- START ANKI -------------------- */
static bool anki_accessible(void) {
    if (!ac_check_connection()) {
        err("Cannot connect to Anki. Is Anki running?");
        return false;
    }
    return true;
}

static void clipboard_changed(GtkClipboard *clipboard, GdkEvent *event, gpointer user_data) {
    DpApplication *app = user_data;

    s8 sentence = fromcstr_(gtk_clipboard_wait_for_text(clipboard));
    // if (!sentence.len) // TODO: Handle?
    dictentry entry_to_add = dm_get_currently_visible(app->dict_manager);
    s8 lookup = app->lookup_word;

    create_ankicard(lookup, sentence, entry_to_add, cfg);

    g_free(sentence.s);
    g_application_quit(G_APPLICATION(app));
}

static void show_copy_sentence_dialog(void) {
    msg("Please copy the context.");
}

void on_add_to_anki_clicked(GtkButton *button, gpointer user_data) {
    if (!anki_accessible())
        return;

    DpApplication *app = user_data;
    gtk_widget_hide(GTK_WIDGET(app->main_window));

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(clipboard_changed), app);

    show_copy_sentence_dialog();
}
/* -------------- END ANKI -------------------- */

void on_pronounce_clicked(GtkButton *button, gpointer user_data) {
    // DpApplication *app = user_data;
}

void on_button_right_clicked(GtkButton *button, gpointer user_data) {
    DpApplication *app = user_data;
    dm_increment(&app->dict_manager);
    refresh_ui(app);
}

void on_button_left_clicked(GtkButton *button, gpointer user_data) {
    DpApplication *app = user_data;
    dm_decrement(&app->dict_manager);
    refresh_ui(app);
}