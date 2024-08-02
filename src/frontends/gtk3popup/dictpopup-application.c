#include "gtk/gtk.h"

#include "dictpopup-application.h"

#include "callback.h"
#include "dict_state_manager.h"
#include <dictpopup.h>
#include <objects/dict.h>
#include <platformdep/clipboard.h>
#include <utils/messages.h>

G_DEFINE_TYPE(DpApplication, dp_application, GTK_TYPE_APPLICATION)

void perform_lookup(DpApplication *app, s8 word);

static void dp_application_dispose(GObject *object) {
    // DpApplication *self = DP_APPLICATION(object);
}

static void dictpopup_activate(GApplication *app) {
    DpApplication *self = DP_APPLICATION(app);
    gtk_widget_show_all(GTK_WIDGET(self->main_window));

    if (!self->lookup_word.len) {
        self->lookup_word = get_selection();
    }
    perform_lookup(self, self->lookup_word);
}

static void move_win_to_mouse_ptr(GtkWindow *win) {
    GdkDisplay *display = gdk_display_get_default();
    GdkDevice *device = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
    gint x, y;
    gdk_device_get_position(device, NULL, &x, &y);
    gtk_window_move(win, x, y);
}

static void quit_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    GApplication *app = G_APPLICATION(data);
    g_application_quit(app);
}

static GActionEntry app_entries[] = {
    {"quit", quit_activated, NULL, NULL, NULL, {0}},
};

static void dictpopup_startup(GApplication *app) {
    const char *const close_window_accels[] = {"q", "Escape", NULL};

    DpApplication *self = DP_APPLICATION(app);
    G_APPLICATION_CLASS(dp_application_parent_class)->startup(app);

    GtkBuilder *builder =
        gtk_builder_new_from_resource("/com/github/Ajatt-Tools/dictpopup/main-window.ui");

    self->main_window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    self->settings_button = GTK_WIDGET(gtk_builder_get_object(builder, "settings_button"));
    self->definition_textbuffer =
        GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "dictionary_textbuffer"));
    self->dictionary_number_lbl = GTK_LABEL(gtk_builder_get_object(builder, "lbl_dictnum"));
    self->dictname_lbl = GTK_LABEL(gtk_builder_get_object(builder, "dictname_lbl"));
    self->current_word_lbl = GTK_LABEL(gtk_builder_get_object(builder, "lbl_cur_reading"));
    self->frequency_lbl = GTK_LABEL(gtk_builder_get_object(builder, "frequency_lbl"));

    gtk_builder_connect_signals(builder, self);
    g_object_unref(builder);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(provider, "/com/github/Ajatt-Tools/dictpopup/style.css");
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    move_win_to_mouse_ptr(self->main_window);

    gtk_window_set_application(GTK_WINDOW(self->main_window), GTK_APPLICATION(app));

    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit", close_window_accels);
}

static void dp_application_init(DpApplication *self) {
    g_mutex_init(&self->dict_manager_mutex);
    dict_manager_init(&self->dict_manager);
    self->lookup_word = (s8){0};

    g_action_map_add_action_entries(G_ACTION_MAP(self), app_entries, G_N_ELEMENTS(app_entries),
                                    self);
}

static int dp_application_command_line(GApplication *app, GApplicationCommandLine *cmdline) {
    DpApplication *self = DP_APPLICATION(app);

    int argc;
    char **argv = g_application_command_line_get_arguments(cmdline, &argc);

    for (int i = 0; i < argc; i++) {
        if (i == 0) {
            continue;
        }

        self->lookup_word = s8dup(fromcstr_(argv[i]));
    }

    g_strfreev(argv);
    g_application_activate(app);
    return EXIT_SUCCESS;
}

static void dp_application_class_init(DpApplicationClass *klass) {
    G_APPLICATION_CLASS(klass)->activate = dictpopup_activate;
    G_APPLICATION_CLASS(klass)->startup = dictpopup_startup;
    G_APPLICATION_CLASS(klass)->command_line = dp_application_command_line;
}

/* ------------ START REFRESH UI ------------------ */
static void refresh_definition_with(DpApplication *app, dictentry de) {
    gtk_text_buffer_set_text(app->definition_textbuffer, (char *)de.definition.s,
                             (gint)de.definition.len);

    // GtkTextIter start, end;
    // gtk_text_buffer_get_bounds(app->definition_textbuffer, &start, &end);
    // gtk_text_buffer_apply_tag_by_name(app->definition_textbuffer, "x-large", &start, &end);
}

static void refresh_dictionary_number(DpApplication *app) {
    char tmp[20] = {0};
    snprintf_safe(tmp, arrlen(tmp), "%li/%li", dm_get_current_index(app->dict_manager) + 1,
                  dm_get_total_num_of_entries(app->dict_manager));
    gtk_label_set_text(app->dictionary_number_lbl, tmp);
}

static void refresh_dictname_with(DpApplication *app, dictentry de) {
    gtk_label_set_text(app->dictname_lbl, (char *)de.dictname.s);
}

static void refresh_title_with(DpApplication *app, dictentry de) {
    _drop_(frees8) s8 title =
        concat(S("dictpopup - "), de.reading, S("【"), de.kanji, S("】from "), de.dictname);
    gtk_window_set_title(app->main_window, (char *)title.s);
}

static void set_frequency_to(DpApplication *app, u32 freq) {
    char tmp[20] = {0};
    const char *freqstr = freq == 0 ? "" : tmp;
    if (freqstr == tmp)
        snprintf_safe(tmp, arrlen(tmp), "%i", freq);
    gtk_label_set_text(app->frequency_lbl, freqstr);
}

static void refresh_current_word_with(DpApplication *app, dictentry de) {
    _drop_(frees8) s8 txt = {0};
    if (de.kanji.len && de.reading.len) {
        if (s8equals(de.kanji, de.reading))
            txt = s8dup(de.reading.len ? de.reading : de.kanji);
        else
            txt = concat(de.reading, S("【"), de.kanji, S("】"));
    } else if (de.reading.len)
        txt = s8dup(de.reading);
    else if (de.kanji.len)
        txt = s8dup(de.kanji);
    else
        txt = s8dup(S(""));

    gtk_label_set_text(app->current_word_lbl, (char *)txt.s);
}

static gboolean _refresh_ui(gpointer user_data) {
    DpApplication *app = user_data;

    g_mutex_lock(&app->dict_manager_mutex);
    dictentry current_entry = dm_get_currently_visible(app->dict_manager);

    refresh_dictionary_number(app);

    refresh_definition_with(app, current_entry);
    refresh_dictname_with(app, current_entry);
    refresh_title_with(app, current_entry);
    refresh_current_word_with(app, current_entry);
    set_frequency_to(app, current_entry.frequency);

    g_mutex_unlock(&app->dict_manager_mutex);

    return G_SOURCE_REMOVE;
}

void refresh_ui(DpApplication *app) {
    gdk_threads_add_idle(_refresh_ui, app);
}
/* ------------ END REFRESH UI ------------------ */

/* ------------- START DICTIONARY LOOKUP --------------------- */

static void dictionary_lookup_thread(GTask *task, gpointer source_object, gpointer task_data,
                                     GCancellable *cancellable) {
    s8 *lookup_str = task_data;
    DictLookup dict_lookup = dictionary_lookup(*lookup_str, cfg);

    if (g_task_return_error_if_cancelled(task)) {
        free(lookup_str->s);
        free(lookup_str);
        dict_free(dict_lookup.dict);
        return;
    }

    DictLookup *dict_lookup_ptr = new (DictLookup, 1);
    *dict_lookup_ptr = dict_lookup;
    g_task_return_pointer(task, dict_lookup_ptr, (GDestroyNotify)dict_lookup_free);
}

static void lookup_dictionary_async(DpApplication *app, const s8 word, GCancellable *cancellable,
                                    GAsyncReadyCallback callback) {
    GTask *task = g_task_new(app, cancellable, callback, NULL);
    g_task_set_task_data(task, s8dup_ptr(word), g_free);
    g_task_run_in_thread(task, dictionary_lookup_thread);
    g_object_unref(task);
}

static DictLookup *lookup_dictionary_finish(DpApplication *app, GAsyncResult *result,
                                            GError **error) {
    return g_task_propagate_pointer(G_TASK(result), error);
}

static void lookup_dictionary_callback(GObject *source_object, GAsyncResult *res,
                                       gpointer user_data) {
    DpApplication *app = DP_APPLICATION(source_object);
    GError *error = NULL;
    DictLookup *new_dict_lookup = lookup_dictionary_finish(app, res, &error);

    if (error) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            err("%s", error->message);
        }
        g_error_free(error);
        return;
    }

    g_mutex_lock(&app->dict_manager_mutex);
    dm_replace_dict(&app->dict_manager, new_dict_lookup->dict);
    app->lookup_word = new_dict_lookup->lookup;
    g_mutex_unlock(&app->dict_manager_mutex);

    // Update UI with new dictionary contents
    refresh_ui(app);

    free(new_dict_lookup);
}

void perform_lookup(DpApplication *app, const s8 word) {
    // Cancel any ongoing lookup
    if (app->current_cancellable) {
        g_cancellable_cancel(app->current_cancellable);
        g_clear_object(&app->current_cancellable);
    }
    app->current_cancellable = g_cancellable_new();

    lookup_dictionary_async(app, word, app->current_cancellable, lookup_dictionary_callback);
}