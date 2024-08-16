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
    gtk_widget_show_all(g_object_new(DP_TYPE_PREFERENCES_WINDOW, "settings", dp->settings, NULL));
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
static s8 get_text_selection(DpApplication *app) {
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(app->definition_textbuffer, &start, &end))
        return fromcstr_(gtk_text_buffer_get_text(app->definition_textbuffer, &start, &end, FALSE));
    return (s8){0};
}

static void sentence_selected(GtkClipboard *clipboard, GdkEvent *event, gpointer user_data) {
    DpApplication *app = user_data;

    s8 sentence = fromcstr_(gtk_clipboard_wait_for_text(clipboard));
    dictentry entry_to_add = dm_get_currently_visible(app->dict_manager);

    s8 text_selection = get_text_selection(app);
    if (text_selection.len > 0) {
        frees8(&entry_to_add.definition);
        entry_to_add.definition = text_selection;
    }

    AnkiConfig anki_cfg = dp_settings_get_anki_settings(app->settings);
    create_ankicard(app->lookup_str, sentence, entry_to_add, anki_cfg);

    g_free(sentence.s);
    g_application_quit(G_APPLICATION(app));
}

static void show_copy_sentence_dialog(void) {
    msg("Please copy the context.");
}

void add_to_anki_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    if (!ac_check_connection()) {
        err("Cannot connect to Anki. Is Anki running?");
        return;
    }

    DpApplication *app = data;
    gtk_widget_hide(GTK_WIDGET(app->main_window));

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(sentence_selected), app);
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