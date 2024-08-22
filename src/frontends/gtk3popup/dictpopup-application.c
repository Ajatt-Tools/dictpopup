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
#include <utils/dp_profile.h>
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

    if (!db_check_exists(db_get_dbpath())) {
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

    for (int i = 1; i < argc; i++) {
        self->initial_lookup_str = s8dup(fromcstr_(argv[i]));
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
          "You need to create one first with the command line application 'dictpopup-create'.");
    ui_manager_set_error(&app->ui_manager, no_database_text);
}

/* ---------------- START OBJECT FUNCTIONS ---------------------- */
s8 dp_get_lookup_str(DpApplication *app) {
    // TODO: Mutex
    s8 lookup_copy = s8dup(app->initial_lookup_str);
    return lookup_copy;
}

void dp_swap_initial_lookup(DpApplication *app, s8 new_inital_lookup) {
    // TODO mutex
    frees8(&app->initial_lookup_str);
    app->initial_lookup_str = new_inital_lookup;
}

static void dp_swap_actual_lookup(DpApplication *app, s8 new_actual_lookup) {
    // TODO mutex
    frees8(&app->actual_lookup_str);
    app->actual_lookup_str = new_actual_lookup;
}

void dp_swap_dict_lookup(DpApplication *app, DictLookup new_dict_lookup) {
    pm_swap_dict(&app->page_manager, new_dict_lookup.dict);
    ui_refresh(&app->ui_manager, &app->page_manager);

    dp_swap_actual_lookup(app, new_dict_lookup.lookup);
}

Word dp_get_copy_of_current_word(DpApplication *app) {
    return pm_get_current_word(&app->page_manager);
}
/* ---------------- END OBJECT FUNCTIONS ---------------------- */