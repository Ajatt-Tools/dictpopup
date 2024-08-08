#include "dictpopup-application.h"
#include "gtk/gtk.h"
#include <stdbool.h>

#include "callbacks.h"

#include "dp-settings.h"

#include <anki.h>
#include <ankiconnectc.h>
#include <dictionary_lookup.h>
#include <jppron/jppron.h>
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

void on_anki_status_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    DpApplication *app = DP_APPLICATION(user_data);

    Word *current_word = dp_get_copy_of_current_word(app);

    char *errormsg = NULL;
    ac_gui_search(dp_settings_get_anki_deck(app->settings),
                  dp_settings_get_anki_search_field(app->settings), (char *)current_word->kanji.s,
                  &errormsg);
    if (errormsg) {
        err("%s", errormsg);
        free(errormsg);
    }

    word_ptr_free(current_word);
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
    s8 lookup = app->lookup_str;

    // TODO: Put into dp-settings
    AnkiConfig cfg = {0};
    cfg.deck = dp_settings_get_anki_deck(app->settings);
    cfg.notetype = dp_settings_get_anki_notetype(app->settings);
    cfg.fieldnames = dp_settings_get_anki_fieldnames(app->settings);
    cfg.fieldMapping = dp_settings_get_anki_fieldentries(app->settings, &cfg.numFields);

    create_ankicard(lookup, sentence, entry_to_add, cfg);

    g_free(sentence.s);
    g_application_quit(G_APPLICATION(app));
}

static void show_copy_sentence_dialog(void) {
    msg("Please copy the context.");
}

void add_to_anki_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    if (!anki_accessible())
        return;

    DpApplication *app = data;
    gtk_widget_hide(GTK_WIDGET(app->main_window));

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(clipboard_changed), app);

    show_copy_sentence_dialog();
}
/* -------------- END ANKI -------------------- */

/* --------------- START JPPRON --------------- */
static void *jppron_thread(void *arg) {
    Word *word = (Word *)arg;
    jppron(word->kanji, word->reading, 0);
    word_ptr_free(word);
    return NULL;
}

void pronounce_current_word(DpApplication *app) {

    Word *word = dp_get_copy_of_current_word(app);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, jppron_thread, word) != 0) {
        dbg("Failed to create pronunciation thread");
        word_ptr_free(word);
    } else {
        pthread_detach(thread_id);
    }
}

void pronounce_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = DP_APPLICATION(data);
    pronounce_current_word(app);
}
/* --------------- END JPPRON --------------- */

void next_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;
    dm_increment(&app->dict_manager);
    refresh_ui(app);
}

void previous_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;
    dm_decrement(&app->dict_manager);
    refresh_ui(app);
}

void quit_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    GApplication *app = G_APPLICATION(data);
    g_application_quit(app);
}

Color map_ac_status_to_color(AnkiCollectionStatus status) {
    switch (status) {
        case AC_ERROR:
            return GREY;
        case AC_EXISTS:
            return ANKI_GREEN;
        case AC_EXISTS_NEW:
            return ANKI_BLUE;
        case AC_EXISTS_SUSPENDED:
            return ORANGE;
        case AC_DOES_NOT_EXIST:
            return ANKI_RED;
    }

    return GREY;
}

/* --------------- START DICTIONARY LOOKUP --------------- */
struct DictLookupArgs {
    DpApplication *app;
    DictpopupConfig cfg;
};

static void *dict_lookup_thread(void *voidarg) {
    struct DictLookupArgs *args = (struct DictLookupArgs *)voidarg;

    s8 *lookup = dp_get_lookup_str(args->app);

    assert(lookup);
    DictLookup dict_lookup = dictionary_lookup(*lookup, args->cfg);
    dp_swap_dict_lookup(args->app, dict_lookup);

    free(args);
    return NULL;
}

void dict_lookup_async(DpApplication *app) {
    struct DictLookupArgs *args = new (struct DictLookupArgs, 1);
    *args = (struct DictLookupArgs){
        .app = app,
        // TODO: Put this into dp-settings
        .cfg = (DictpopupConfig){
            .database_dir = dp_settings_get_database_path(app->settings),
            .sort_dict_entries = dp_settings_get_sort_entries(app->settings),
            .dict_sort_order = dp_settings_get_dict_sort_order(app->settings),
            .nuke_whitespace_of_lookup = dp_settings_get_nuke_whitespace_of_lookup(app->settings),
            .fallback_to_mecab_conversion = dp_settings_get_mecab_conversion(app->settings),
            .lookup_longest_matching_prefix =
                dp_settings_get_lookup_longest_matching_prefix(app->settings),
        }};

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, dict_lookup_thread, args) != 0) {
        dbg("Failed to create pronunciation thread");
        free(args);
    } else {
        pthread_detach(thread_id);
    }
}
/* --------------- END DICTIONARY LOOKUP --------------- */