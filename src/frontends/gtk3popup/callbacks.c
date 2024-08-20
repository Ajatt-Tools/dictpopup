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
static s8 get_text_selection(DpApplication *app) {
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(app->definition_textbuffer, &start, &end))
        return fromcstr_(gtk_text_buffer_get_text(app->definition_textbuffer, &start, &end, FALSE));
    return (s8){0};
}

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

    dictentry entry_to_add = dm_get_currently_visible(app->dict_manager);
    if (text_selection.len > 0) {
        frees8(&entry_to_add.definition);
        entry_to_add.definition = text_selection;
    }

    AnkiConfig anki_cfg = dp_settings_get_anki_settings(app->settings);
    create_ankicard(app->lookup_str, sentence, entry_to_add, anki_cfg);

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
static void add_to_anki_from_clipboard(GtkMenuItem *self, gpointer user_data) {
    DpApplication *app = DP_APPLICATION(user_data);

    if (!ac_check_connection()) {
        err("Cannot connect to Anki. Is Anki running?");
        return;
    }

    gtk_widget_hide(GTK_WIDGET(app->main_window));

    struct SelectionData *selection_data = new (struct SelectionData, 1);
    selection_data->app = app;
    selection_data->definition_override = get_clipboard();

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(sentence_selected), selection_data);

    show_copy_sentence_dialog();
}

void add_to_anki_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;

    if (!ac_check_connection()) {
        err("Cannot connect to Anki. Is Anki running?");
        return;
    }

    gtk_widget_hide(GTK_WIDGET(app->main_window));

    struct SelectionData *selection_data = new (struct SelectionData, 1);
    selection_data->app = app;
    selection_data->definition_override = get_text_selection(app);

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(sentence_selected), selection_data);

    show_copy_sentence_dialog();
}

static void show_anki_button_right_click_menu(DpApplication *self) {
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *menu_item = gtk_menu_item_new_with_label("Add with clipboard content as definition");
    g_signal_connect(menu_item, "activate", G_CALLBACK(add_to_anki_from_clipboard), self);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), self->btn_add_to_anki, GDK_GRAVITY_SOUTH,
                             GDK_GRAVITY_NORTH_WEST, NULL);
}
/* -------------- END ANKI -------------------- */

/* --------------- START JPPRON --------------- */
static void *jppron_thread(void *arg) {
    Word *word = (Word *)arg;
    jppron(*word, 0);
    word_free(*word);
    return NULL;
}

void pronounce_current_word(DpApplication *app) {

    Word *word = new (Word, 1);
    *word = dp_get_copy_of_current_word(app);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, jppron_thread, word) != 0) {
        dbg("Failed to create pronunciation thread");
        word_free(*word);
    } else {
        pthread_detach(thread_id);
    }
}

void pronounce_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = DP_APPLICATION(data);
    pronounce_current_word(app);
}

static void play_audio_for_pronfile(GtkMenuItem *menuitem, gpointer user_data) {
    Pronfile *pronfile = (Pronfile *)user_data;
    play_audio_async(pronfile->filepath);
}

static char *create_label_for_pronfile(Pronfile pronfile) {
    s8 origin = pronfile.fileinfo.origin;
    s8 pitch_pattern = pronfile.fileinfo.pitch_pattern;
    char *label = g_strdup_printf("%.*s%s%.*s", (int)pitch_pattern.len, (char *)pitch_pattern.s,
                                  origin.len && pitch_pattern.len ? " - " : "", (int)origin.len,
                                  (char *)origin.s);

    return label;
}

static void show_pronunciation_button_right_click_menu(DpApplication *self) {
    GtkWidget *menu = gtk_menu_new();

    for (size_t i = 0; i < buf_size(self->pronfiles); i++) {
        char *label = create_label_for_pronfile(self->pronfiles[i]);
        GtkWidget *menu_item = gtk_menu_item_new_with_label(label);
        g_signal_connect(menu_item, "activate", G_CALLBACK(play_audio_for_pronfile), &self->pronfiles[i]);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), self->btn_pronounce, GDK_GRAVITY_SOUTH,
                             GDK_GRAVITY_NORTH_WEST, NULL);
}
/* --------------- END JPPRON --------------- */

void next_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;
    if (dm_increment(&app->dict_manager))
        refresh_ui(app);
}

void previous_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    DpApplication *app = data;
    if (dm_decrement(&app->dict_manager))
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

static gboolean enable_buttons_idle(gpointer user_data) {
    DpApplication *app = (DpApplication *)user_data;
    enable_dictionary_buttons(app);
    return G_SOURCE_REMOVE;
}

static void *dict_lookup_thread(void *voidarg) {
    struct DictLookupArgs *args = (struct DictLookupArgs *)voidarg;

    s8 lookup = dp_get_lookup_str(args->app);
    DictLookup dict_lookup = dictionary_lookup(lookup, args->cfg);

    if (isEmpty(dict_lookup.dict)) {
        set_no_dictentry_found(args->app);
    } else {
        dp_swap_dict_lookup(args->app, dict_lookup);
        gdk_threads_add_idle(enable_buttons_idle, args->app);
    }

    free(args);
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

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, dict_lookup_thread, args) != 0) {
        dbg("Failed to create pronunciation thread");
        free(args);
    } else {
        pthread_detach(thread_id);
    }
}
/* --------------- END DICTIONARY LOOKUP --------------- */

/* --------------- START CALLBACKS ------------------ */
void on_settings_button_clicked(GtkButton *button, gpointer user_data) {
    DpApplication *dp = (DpApplication *)user_data;
    gtk_widget_show_all(g_object_new(DP_TYPE_PREFERENCES_WINDOW, "settings", dp->settings, NULL));
}

void on_anki_status_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    DpApplication *app = DP_APPLICATION(user_data);

    _drop_(word_free) Word current_word = dp_get_copy_of_current_word(app);

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
        show_anki_button_right_click_menu(app);
        return TRUE;
    }
    return FALSE;
}

gboolean on_pronounce_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        DpApplication *app = DP_APPLICATION(user_data);
        show_pronunciation_button_right_click_menu(app);
        return TRUE;
    }
    return FALSE;
}