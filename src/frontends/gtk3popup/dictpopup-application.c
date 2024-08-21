#include "gtk/gtk.h"

#include "dictpopup-application.h"

#include "callbacks.h"
#include "dict_state_manager.h"
#include "dp-settings.h"

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
static gboolean dp_update_exists_dot(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static void set_no_lookup_string(DpApplication *app);
static void set_database_not_found(DpApplication *app);
static void disable_button(GtkWidget *button);
static void disable_dictionary_buttons(DpApplication *app);

static void dp_application_dispose(GObject *object) {
    DpApplication *self = DP_APPLICATION(object);

    g_clear_object(&self->settings);
    frees8(&self->lookup_str);

    G_OBJECT_CLASS(dp_application_parent_class)->dispose(object);
}

static bool can_lookup(DpApplication *app) {
    if (!app->lookup_str.len) {
        set_no_lookup_string(app);
        return false;
    }

    // TODO: Fix
    _drop_(frees8) s8 dbpath = db_get_dbpath();
    if (!db_check_exists(dbpath)) {
        set_database_not_found(app);
        return false;
    }

    return true;
}

static void dictpopup_activate(GApplication *app) {
    DpApplication *self = DP_APPLICATION(app);

    disable_dictionary_buttons(self);

    if (!self->lookup_str.len)
        self->lookup_str = get_selection();

    if (can_lookup(self))
        dict_lookup_async(self);

    gtk_widget_show_all(GTK_WIDGET(self->main_window));
}

static void move_win_to_mouse_ptr(GtkWindow *win) {
    GdkDisplay *display = gdk_display_get_default();
    GdkDevice *device = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
    gint x, y;
    gdk_device_get_position(device, NULL, &x, &y);
    gtk_window_move(win, x, y);
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

static void dictpopup_startup(GApplication *app) {
    const char *const close_window_accels[] = {"q", "Escape", NULL};
    const char *const next_definition_accels[] = {"s", NULL};
    const char *const previous_definition_accels[] = {"a", NULL};
    const char *const pronounce_accels[] = {"p", "r", NULL};
    const char *const add_to_anki_accels[] = {"<Ctrl>s", NULL};

    DpApplication *self = DP_APPLICATION(app);
    G_APPLICATION_CLASS(dp_application_parent_class)->startup(app);

    GtkBuilder *builder =
        gtk_builder_new_from_resource("/com/github/Ajatt-Tools/dictpopup/main-window.ui");

    /* buttons */
    self->btn_previous = GTK_WIDGET(gtk_builder_get_object(builder, "btn_left"));
    self->btn_next = GTK_WIDGET(gtk_builder_get_object(builder, "btn_right"));
    self->btn_pronounce = GTK_WIDGET(gtk_builder_get_object(builder, "pronounce_btn"));
    self->btn_add_to_anki = GTK_WIDGET(gtk_builder_get_object(builder, "add_to_anki_btn"));
    /* ------- */

    GtkWidget *menu_button = GTK_WIDGET(gtk_builder_get_object(builder, "menu_button"));
    GtkWidget *popover_menu = GTK_WIDGET(gtk_builder_get_object(builder, "popover_menu"));
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), popover_menu);

    self->main_window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    self->definition_textbuffer =
        GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "dictionary_textbuffer"));
    self->dictionary_number_lbl = GTK_LABEL(gtk_builder_get_object(builder, "lbl_dictnum"));
    self->dictname_lbl = GTK_LABEL(gtk_builder_get_object(builder, "dictname_lbl"));
    self->current_word_lbl = GTK_LABEL(gtk_builder_get_object(builder, "lbl_cur_reading"));
    self->frequency_lbl = GTK_LABEL(gtk_builder_get_object(builder, "frequency_lbl"));

    self->anki_status_dot = GTK_WIDGET(gtk_builder_get_object(builder, "anki_status_dot"));
    g_signal_connect(self->anki_status_dot, "draw", G_CALLBACK(dp_update_exists_dot), self);
    gtk_widget_add_events(self->anki_status_dot, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(self->anki_status_dot, "button-press-event",
                     G_CALLBACK(on_anki_status_clicked), self);

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
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.next-definition",
                                          next_definition_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.previous-definition",
                                          previous_definition_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.pronounce", pronounce_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.add-to-anki",
                                          add_to_anki_accels);
}

static void dp_application_init(DpApplication *self) {
    self->settings = dp_settings_new();
    g_mutex_init(&self->dict_manager_mutex);
    dict_manager_init(&self->dict_manager);
    self->lookup_str = (s8){0};
    self->anki_exists_status = 0;

    g_action_map_add_action_entries(G_ACTION_MAP(self), app_entries, G_N_ELEMENTS(app_entries),
                                    self);
}

static int dp_application_command_line(GApplication *app, GApplicationCommandLine *cmdline) {
    DpApplication *self = DP_APPLICATION(app);

    int argc;
    char **argv = g_application_command_line_get_arguments(cmdline, &argc);

    for (int i = 1; i < argc; i++) {
        self->lookup_str = s8dup(fromcstr_(argv[i]));
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

static void disable_button(GtkWidget *button) {
    gtk_widget_set_sensitive(button, FALSE);
}

void disable_dictionary_buttons(DpApplication *app) {
    disable_button(app->btn_previous);
    disable_button(app->btn_next);
    disable_button(app->btn_add_to_anki);
}

static void enable_button(GtkWidget *button) {
    gtk_widget_set_sensitive(button, TRUE);
}

void enable_dictionary_buttons(DpApplication *app) {
    enable_button(app->btn_previous);
    enable_button(app->btn_next);
    enable_button(app->btn_add_to_anki);
}

static void set_text(DpApplication *app, s8 text) {
    gtk_text_buffer_set_text(app->definition_textbuffer, (char *)text.s, (gint)text.len);
}

static void set_no_lookup_string(DpApplication *app) {
    set_text(app, S("No lookup provided."));
}

void set_no_dictentry_found(DpApplication *app) {
    set_text(app, S("No dictionary entry found."));
}

static void set_database_not_found(DpApplication *app) {
    s8 no_database_text =
        S("No database found. "
          "You need to create one first with the command line application 'dictpopup-create'.");
    set_text(app, no_database_text);
}

/* ---------------- START OBJECT FUNCTIONS ---------------------- */
s8 dp_get_lookup_str(DpApplication *app) {
    g_mutex_lock(&app->dict_manager_mutex);

    // s8 *lookup_copy = new (s8, 1);
    s8 lookup_copy = s8dup(app->lookup_str);

    g_mutex_unlock(&app->dict_manager_mutex);

    return lookup_copy;
}

/* ------------ START PRONUNCIATION ------------------- */
static void initiate_pronunciation(DpApplication *self) {
    _drop_(word_free) Word current_word = dp_get_copy_of_current_word(self);
    self->pronfiles = get_pronfiles_for(current_word);

    if (buf_size(self->pronfiles) != 0) {
        enable_button(self->btn_pronounce);

        if (dp_settings_get_pronounce_on_startup(self->settings)) {
            play_audio_async(self->pronfiles[0].filepath);
        }
    } else
        disable_button(self->btn_pronounce);
}
/* ---------------- END PRONUNCIATION -------------- */
static void initiate_anki_indicator(DpApplication *self) {
    char *deck = dp_settings_get_anki_deck(self->settings);
    char *search_field = dp_settings_get_anki_search_field(self->settings);
    Word word = dp_get_copy_of_current_word(self);

    self->anki_exists_status = ac_check_exists(deck, search_field, (char *)word.kanji.s, 0);

    gtk_widget_queue_draw(self->anki_status_dot);
}

static void on_lookup_completed(DpApplication *app) {
    refresh_ui(app);

    set_end_time_now(); // Not quite accuracte possibly

    initiate_pronunciation(app);
    initiate_anki_indicator(app);
}

void dp_swap_dict_lookup(DpApplication *app, DictLookup new_dict_lookup) {
    g_mutex_lock(&app->dict_manager_mutex);

    dm_swap_dict(&app->dict_manager, new_dict_lookup.dict);

    frees8(&app->lookup_str);
    app->lookup_str = new_dict_lookup.lookup;

    g_mutex_unlock(&app->dict_manager_mutex);

    on_lookup_completed(app);
}

Word dp_get_copy_of_current_word(DpApplication *app) {
    g_mutex_lock(&app->dict_manager_mutex);

    dictentry current_entry = dm_get_currently_visible(app->dict_manager);
    Word word_copy =
        word_dup((Word){.kanji = current_entry.kanji, .reading = current_entry.reading});

    g_mutex_unlock(&app->dict_manager_mutex);

    return word_copy;
}

static void update_exists_dot_with_color(GtkWidget *widget, cairo_t *cr, Color color) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    int size = MIN(width, height) * 2 / 3;
    int x = (width - size) / 2;
    int y = (height - size) / 2;

    cairo_set_source_rgb(cr, color.red, color.green, color.blue);
    cairo_arc(cr, x + size / 2, y + size / 2, size / 2, 0, 2 * G_PI);
    cairo_fill(cr);
}

static gboolean dp_update_exists_dot(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    DpApplication *app = DP_APPLICATION(user_data);

    Color color = map_ac_status_to_color(app->anki_exists_status);
    update_exists_dot_with_color(widget, cr, color);

    return FALSE;
}
/* ---------------- END OBJECT FUNCTIONS ---------------------- */

/* ------------ START REFRESH UI ------------------ */
static void refresh_definition_with_entry(DpApplication *app, dictentry de) {
    gtk_text_buffer_set_text(app->definition_textbuffer, (char *)de.definition.s,
                             (gint)de.definition.len);
}

static void refresh_dictionary_number(DpApplication *app) {
    char tmp[20] = {0};
    snprintf_safe(tmp, arrlen(tmp), "%li/%li", dm_get_current_index(app->dict_manager) + 1,
                  dm_get_total_num_of_entries(app->dict_manager));
    gtk_label_set_text(app->dictionary_number_lbl, tmp);
}

static void refresh_dictname_with_entry(DpApplication *app, dictentry de) {
    gtk_label_set_text(app->dictname_lbl, (char *)de.dictname.s);
}

static void refresh_title_with_entry(DpApplication *app, dictentry de) {
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

static void refresh_current_word_with_entry(DpApplication *app, dictentry de) {
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

    refresh_definition_with_entry(app, current_entry);
    refresh_dictname_with_entry(app, current_entry);
    refresh_title_with_entry(app, current_entry);
    refresh_current_word_with_entry(app, current_entry);
    set_frequency_to(app, current_entry.frequency);

    g_mutex_unlock(&app->dict_manager_mutex);

    return G_SOURCE_REMOVE;
}

void refresh_ui(DpApplication *app) {
    gdk_threads_add_idle(_refresh_ui, app);
}
/* ------------ END REFRESH UI ------------------ */