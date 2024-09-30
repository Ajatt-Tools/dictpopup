#include "dictpopup-application.h"
#include "gtk/gtk.h"
#include <stdbool.h>

#include "callbacks.h"

#include "dp-settings.h"

#include <anki.h>
#include <ankiconnectc.h>
#include <dictionary_lookup.h>
#include <jppron/jppron.h>
#include <platformdep/audio.h>
#include <platformdep/clipboard.h>
#include <utils/messages.h>

/* -------------- START ANKI -------------------- */
struct SelectionData {
    DpApplication *app;
    s8 definition_override;
};

static void sentence_selected(GtkClipboard *clipboard, GdkEvent *event, gpointer user_data) {
    struct SelectionData *data = (struct SelectionData *)user_data;
    DpApplication *app = data->app;
    s8 text_selection = data->definition_override;

    s8 sentence = fromcstr_(gtk_clipboard_wait_for_text(clipboard));
    if (dp_settings_get_nuke_whitespace_of_sentence(app->settings)) {
        nuke_whitespace(&sentence);
    }

    Dictentry entry_to_add = dp_get_current_dictentry(app);
    if (text_selection.len > 0) {
        frees8(&entry_to_add.definition);
        entry_to_add.definition = text_selection;
    }

    AnkiConfig anki_cfg = dp_settings_get_anki_settings(app->settings);
    create_ankicard(app->actual_lookup_str, sentence, entry_to_add, anki_cfg);

    // TODO: Cleanup
    free(anki_cfg.fieldmapping.field_content);
    g_strfreev(anki_cfg.fieldmapping.field_names);
    dictentry_free(entry_to_add);
    g_free(sentence.s);
    free(data);
    g_application_quit(G_APPLICATION(app));
}

static void show_copy_sentence_dialog(void) {
    msg("Please copy the context.");
}

// TODO TODO: Remove duplication
void add_to_anki_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;

    if (!ac_check_connection()) {
        err("Cannot connect to Anki. Is Anki running?");
        return;
    }

    ui_manager_hide_window(&app->ui_manager);

    struct SelectionData *selection_data = new (struct SelectionData, 1);
    selection_data->app = app;
    selection_data->definition_override = ui_manager_get_text_selection(&app->ui_manager);

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(sentence_selected), selection_data);

    show_copy_sentence_dialog();
}

static void on_add_to_anki_from_clipboard(GtkMenuItem *self, gpointer user_data) {
    DpApplication *app = DP_APPLICATION(user_data);

    if (!ac_check_connection()) {
        err("Cannot connect to Anki. Is Anki running?");
        return;
    }

    ui_manager_hide_window(&app->ui_manager);

    struct SelectionData *selection_data = new (struct SelectionData, 1);
    selection_data->app = app;
    selection_data->definition_override = get_clipboard();

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(sentence_selected), selection_data);

    show_copy_sentence_dialog();
}
/* -------------- END ANKI -------------------- */
void pronounce_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = DP_APPLICATION(data);
    dp_play_current_pronunciation(app);
}

void next_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;
    if (pm_increment(&app->page_manager))
        ui_refresh(&app->ui_manager, &app->page_manager);
}

void previous_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;
    if (pm_decrement(&app->page_manager))
        ui_refresh(&app->ui_manager, &app->page_manager);
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

static void *dict_lookup_thread(gpointer data) {
    struct DictLookupArgs *args = (struct DictLookupArgs *)data;

    s8 lookup = dp_get_lookup_str(args->app);
    DictLookup dict_lookup = dictionary_lookup(lookup, args->cfg);

    if (isEmpty(dict_lookup.dict)) {
        ui_manager_set_error(&args->app->ui_manager, S("No dictionary entry found"));
    } else {
        dp_swap_dict_lookup(args->app, dict_lookup);
    }

    g_free(args);
    return NULL;
}

void dict_lookup_async(DpApplication *app) {
    struct DictLookupArgs *args = new (struct DictLookupArgs, 1);
    *args = (struct DictLookupArgs){
        .app = app,
        // TODO: Put this into dp-settings
        .cfg = (DictpopupConfig){
            .dict_sort_order = dp_settings_get_dict_sort_order(app->settings),
            .nuke_whitespace_of_lookup = dp_settings_get_nuke_whitespace_of_lookup(app->settings),
            .fallback_to_mecab_conversion = dp_settings_get_mecab_conversion(app->settings),
            .lookup_longest_matching_prefix =
                dp_settings_get_lookup_longest_matching_prefix(app->settings),
        }};

    GError *error = NULL;
    GThread *thread = g_thread_try_new("dict_lookup_thread", dict_lookup_thread, args, &error);
    if (thread == NULL) {
        g_warning("Failed to create dictionary lookup thread: %s", error->message);
        g_error_free(error);
        g_free(args);
    }
}
/* --------------- END DICTIONARY LOOKUP --------------- */

/* -------------------- ACTIONS -------------------- */
static void open_url(const char *url) {
    GError *error = NULL;
    if (!g_app_info_launch_default_for_uri(url, NULL, &error)) {
        g_warning("Failed to open URL with g_app_info_launch_default_for_uri: %s", error->message);
        g_clear_error(&error);
    }
}

void search_massif_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;

    _drop_(word_free) Word word = dp_get_current_word(app);

    _drop_(frees8) s8 url = concat(S("https://massif.la/ja/search?q="), word.kanji);
    open_url((const char *)url.s);
}

static void on_edit_lookup_accept(const char *new_lookup, void *user_data) {
    DpApplication *app = DP_APPLICATION(user_data);
    dp_swap_initial_lookup(app, s8dup(fromcstr_((char *)new_lookup)));
    dict_lookup_async(app);
}

void edit_lookup_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = DP_APPLICATION(data);
    _drop_(frees8) s8 current_lookup_str = dp_get_lookup_str(app);

    ui_manager_show_edit_lookup_dialog(&app->ui_manager, (const char *)current_lookup_str.s,
                                       on_edit_lookup_accept, app);
}

void open_settings_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;
    gtk_widget_show_all(g_object_new(DP_TYPE_PREFERENCES_WINDOW, "settings", app->settings, NULL));
}

/* --------------- START CALLBACKS ------------------ */
void on_anki_status_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    DpApplication *app = user_data;

    _drop_(word_free) Word current_word = dp_get_current_word(app);

    char *errormsg = NULL;
    ac_gui_search(dp_settings_get_anki_deck(app->settings),
                  dp_settings_get_anki_search_field(app->settings), (char *)current_word.kanji.s,
                  &errormsg);
    if (errormsg) {
        err("%s", errormsg);
        free(errormsg);
    }
}

gboolean on_add_to_anki_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        DpApplication *app = DP_APPLICATION(user_data);
        ui_manager_show_anki_button_right_click_menu(&app->ui_manager,
                                                     on_add_to_anki_from_clipboard, app);
        return TRUE;
    }
    return FALSE;
}

gboolean on_pronounce_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        DpApplication *app = DP_APPLICATION(user_data);

        Pronfile *pronfiles = dp_get_current_pronfiles(app);
        show_pronunciation_button_right_click_menu(&app->ui_manager, pronfiles);
        free_pronfile_buffer(pronfiles);

        return TRUE;
    }
    return FALSE;
}