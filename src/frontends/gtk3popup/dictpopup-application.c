#include "gtk/gtk.h"

#include "dictpopup-application.h"

#include "callbacks.h"
#include "dp-settings.h"

#include "ui_manager.h"
#include <ankiconnectc.h>
#include <db.h>
#include <jppron/jppron.h>
#include <objects/dict.h>
#include <platformdep/audio.h>
#include <platformdep/clipboard.h>
#include <utils/messages.h>

G_DEFINE_TYPE(DpApplication, dp_application, GTK_TYPE_APPLICATION)

void perform_lookup(DpApplication *app, s8 word);
static void set_no_lookup_string(DpApplication *app);
static void set_database_not_found(DpApplication *app);

static void dp_application_dispose(GObject *object) {
    DpApplication *self = DP_APPLICATION(object);

    g_clear_object(&self->settings);
    frees8(&self->initial_lookup_str);

    G_OBJECT_CLASS(dp_application_parent_class)->dispose(object);
}

static bool can_lookup(DpApplication *app) {
    if (!app->initial_lookup_str.len) {
        set_no_lookup_string(app);
        return false;
    }

    // TODO: Move?
    if (!db_check_exists()) {
        set_database_not_found(app);
        return false;
    }

    return true;
}

static void dictpopup_activate(GApplication *app) {
    DpApplication *self = DP_APPLICATION(app);

    if (!self->initial_lookup_str.len)
        self->initial_lookup_str = get_selection();

    if (can_lookup(self))
        dict_lookup_async(self);

    ui_manager_show_window(&self->ui_manager);
}

static GActionEntry app_entries[] = {
    {"quit", quit_activated, NULL, NULL, NULL, {0}},
    {"next-definition", next_definition_activated, NULL, NULL, NULL, {0}},
    {"previous-definition", previous_definition_activated, NULL, NULL, NULL, {0}},
    {"pronounce", pronounce_activated, NULL, NULL, NULL, {0}},
    {"add-to-anki", add_to_anki_activated, NULL, NULL, NULL, {0}},
    {"search-massif", search_massif_activated, NULL, NULL, NULL, {0}},
    {"edit-lookup", edit_lookup_activated, NULL, NULL, NULL, {0}},
    {"open-settings", open_settings_activated, NULL, NULL, NULL, {0}},
};

static void init_css(void) {
    dbg("Initting CSS");
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(provider, "/com/github/Ajatt-Tools/dictpopup/style.css");
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static void init_ui(DpApplication *self) {
    GtkBuilder *builder =
        gtk_builder_new_from_resource("/com/github/Ajatt-Tools/dictpopup/main-window.ui");

    ui_manager_init(&self->ui_manager, builder);

    gtk_builder_connect_signals(builder, self);
    g_object_unref(builder);
}

static void init_accels(GApplication *app) {
    const char *const close_window_accels[] = {"q", "Escape", NULL};
    const char *const next_definition_accels[] = {"s", NULL};
    const char *const previous_definition_accels[] = {"a", NULL};
    const char *const pronounce_accels[] = {"p", "r", NULL};
    const char *const add_to_anki_accels[] = {"<Ctrl>s", NULL};

    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit", close_window_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.next-definition",
                                          next_definition_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.previous-definition",
                                          previous_definition_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.pronounce", pronounce_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.add-to-anki",
                                          add_to_anki_accels);
}

static void dictpopup_startup(GApplication *app) {
    DpApplication *self = DP_APPLICATION(app);
    G_APPLICATION_CLASS(dp_application_parent_class)->startup(app);

    init_ui(self);
    init_css();

    ui_manager_set_application(&self->ui_manager, GTK_APPLICATION(app));

    init_accels(app);
}

static void dp_application_init(DpApplication *self) {
    g_mutex_init(&self->dict_data_mutex);
    self->settings = dp_settings_new();
    page_manager_init(&self->page_manager, self->settings);
    self->initial_lookup_str = (s8){0};

    g_action_map_add_action_entries(G_ACTION_MAP(self), app_entries, G_N_ELEMENTS(app_entries),
                                    self);
}

static int dp_application_command_line(GApplication *app, GApplicationCommandLine *cmdline) {
    DpApplication *self = DP_APPLICATION(app);

    int argc;
    char **argv = g_application_command_line_get_arguments(cmdline, &argc);

    if (argc > 1) {
        dp_swap_initial_lookup(self, s8dup(fromcstr_(argv[1])));
    }

    g_strfreev(argv);
    g_application_activate(app);
    return EXIT_SUCCESS;
}

static void dp_application_class_init(DpApplicationClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GApplicationClass *app_class = G_APPLICATION_CLASS(klass);

    object_class->dispose = dp_application_dispose;

    app_class->activate = dictpopup_activate;
    app_class->startup = dictpopup_startup;
    app_class->command_line = dp_application_command_line;
}

static void set_no_lookup_string(DpApplication *app) {
    ui_manager_set_error(&app->ui_manager, S("No lookup provided."));
}

static void set_database_not_found(DpApplication *app) {
    s8 no_database_text =
        S("No database found. "
          "You must first create one in the preferences window.");
    ui_manager_set_error(&app->ui_manager, no_database_text);
}

/* ---------------- START OBJECT FUNCTIONS ---------------------- */
void dp_swap_initial_lookup(DpApplication *self, s8 new_inital_lookup) {
    g_mutex_lock(&self->dict_data_mutex);
    frees8(&self->initial_lookup_str);
    self->initial_lookup_str = new_inital_lookup;
    g_mutex_unlock(&self->dict_data_mutex);
}

static void _dp_swap_actual_lookup_nolock(DpApplication *self, s8 new_actual_lookup) {
    frees8(&self->actual_lookup_str);
    self->actual_lookup_str = new_actual_lookup;
}

static void on_dict_lookup_completed(DpApplication *app) {
    ui_refresh(&app->ui_manager, &app->page_manager);

    if (dp_settings_get_pronounce_on_startup(app->settings))
        dp_play_current_pronunciation(app);
}

void dp_swap_dict_lookup(DpApplication *self, DictLookup new_dict_lookup) {
    g_mutex_lock(&self->dict_data_mutex);
    pm_swap_dict(&self->page_manager, new_dict_lookup.dict);
    _dp_swap_actual_lookup_nolock(self, new_dict_lookup.lookup);
    g_mutex_unlock(&self->dict_data_mutex);

    on_dict_lookup_completed(self);
}

void dp_play_current_pronunciation(DpApplication *self) {
    g_mutex_lock(&self->dict_data_mutex);
    const s8 pron_path = pm_get_path_of_current_pronunciation(&self->page_manager);
    if (pron_path.len)
        play_audio_async(pron_path);
    g_mutex_unlock(&self->dict_data_mutex);
}

Word dp_get_current_word(DpApplication *self) {
    g_mutex_lock(&self->dict_data_mutex);
    Word ret = word_dup(pm_get_current_word(&self->page_manager));
    g_mutex_unlock(&self->dict_data_mutex);
    return ret;
}

_deallocator_(free_pronfile_buffer) Pronfile *dp_get_current_pronfiles(DpApplication *self) {
    Pronfile *ret = 0;

    g_mutex_lock(&self->dict_data_mutex);
    Pronfile *pronfiles = pm_get_current_pronfiles(&self->page_manager);
    for (size_t i = 0; i < buf_size(pronfiles); i++) {
        buf_push(ret, pron_file_dup(pronfiles[i]));
    }
    g_mutex_unlock(&self->dict_data_mutex);

    return ret;
}

s8 dp_get_lookup_str(DpApplication *self) {
    g_mutex_lock(&self->dict_data_mutex);
    s8 lookup_copy = s8dup(self->initial_lookup_str);
    g_mutex_unlock(&self->dict_data_mutex);
    return lookup_copy;
}

Dictentry dp_get_current_dictentry(DpApplication *self) {
    g_mutex_lock(&self->dict_data_mutex);
    Dictentry ret = dictentry_dup(pm_get_current_dictentry(&self->page_manager));
    g_mutex_unlock(&self->dict_data_mutex);
    return ret;
}
/* ---------------- END OBJECT FUNCTIONS ---------------------- */