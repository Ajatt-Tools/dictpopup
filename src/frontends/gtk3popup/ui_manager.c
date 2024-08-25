#include "ui_manager.h"

#include "callbacks.h"
#include "objects/color.h"
#include "platformdep/audio.h"

#include <utils/dp_profile.h>

static gboolean on_refresh_exists_dot(GtkWidget *widget, cairo_t *cr, gpointer user_data);

static void disable_button(GtkWidget *button) {
    gtk_widget_set_sensitive(button, FALSE);
}

static void enable_button(GtkWidget *button) {
    gtk_widget_set_sensitive(button, TRUE);
}

static void disable_all_buttons(UiManager *self) {
    disable_button(self->btn_next);
    disable_button(self->btn_previous);
    disable_button(self->btn_pronounce);
    disable_button(self->btn_add_to_anki);
}

static void move_win_to_mouse_ptr(GtkWindow *win) {
    GdkDisplay *display = gdk_display_get_default();
    GdkDevice *device = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
    gint x, y;
    gdk_device_get_position(device, NULL, &x, &y);
    gtk_window_move(win, x, y);
}

void ui_manager_init(UiManager *self, GtkBuilder *builder) {
    self->current_anki_status_color = GREY;
    self->main_window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    move_win_to_mouse_ptr(self->main_window);

    /* buttons */
    self->btn_previous = GTK_WIDGET(gtk_builder_get_object(builder, "btn_left"));
    self->btn_next = GTK_WIDGET(gtk_builder_get_object(builder, "btn_right"));
    self->btn_pronounce = GTK_WIDGET(gtk_builder_get_object(builder, "pronounce_btn"));
    self->btn_add_to_anki = GTK_WIDGET(gtk_builder_get_object(builder, "add_to_anki_btn"));
    disable_all_buttons(self);
    /* ------- */

    self->main_window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    self->definition_textbuffer =
        GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "dictionary_textbuffer"));
    self->entry_number_label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_dictnum"));
    self->dictname_label = GTK_LABEL(gtk_builder_get_object(builder, "dictname_lbl"));
    self->current_word_label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_cur_reading"));
    self->frequency_label = GTK_LABEL(gtk_builder_get_object(builder, "frequency_lbl"));

    self->anki_status_dot = GTK_WIDGET(gtk_builder_get_object(builder, "anki_status_dot"));
    g_signal_connect(self->anki_status_dot, "draw", G_CALLBACK(on_refresh_exists_dot), self);
}

void ui_manager_set_application(UiManager *self, GtkApplication *app) {
    gtk_window_set_application(self->main_window, app);
}

void _nonnull_ ui_manager_show_window(UiManager *self) {
    gtk_widget_show_all(GTK_WIDGET(self->main_window));
}

void _nonnull_ ui_manager_hide_window(UiManager *self) {
    gtk_widget_hide(GTK_WIDGET(self->main_window));
}

s8 _nonnull_ ui_manager_get_text_selection(UiManager *self) {
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(self->definition_textbuffer, &start, &end))
        return fromcstr_(
            gtk_text_buffer_get_text(self->definition_textbuffer, &start, &end, FALSE));
    return (s8){0};
}

static void refresh_left_right_buttons(UiManager *self, size_t total_num_pages) {
    if (total_num_pages == 1) {
        disable_button(self->btn_next);
        disable_button(self->btn_previous);
    } else {
        enable_button(self->btn_next);
        enable_button(self->btn_previous);
    }
}

static void refresh_exists_dot_with_color(GtkWidget *widget, cairo_t *cr, Color color) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    int size = MIN(width, height) * 2 / 3;
    int x = (width - size) / 2;
    int y = (height - size) / 2;

    cairo_set_source_rgb(cr, color.red, color.green, color.blue);
    cairo_arc(cr, x + size / 2, y + size / 2, size / 2, 0, 2 * G_PI);
    cairo_fill(cr);
}

static gboolean on_refresh_exists_dot(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    UiManager *ui = user_data;

    // Color color = map_ac_status_to_color(pm_get_current_anki_status_nolock(ui));
    Color color = ui->current_anki_status_color;
    refresh_exists_dot_with_color(widget, cr, color);

    return FALSE;
}

static void refresh_definition(UiManager *self, s8 definition) {
    gtk_text_buffer_set_text(self->definition_textbuffer, (char *)definition.s,
                             (gint)definition.len);
}

static void refresh_dictionary_number(UiManager *self, size_t current_index, size_t total_pages) {
    char tmp[20] = {0};
    snprintf_safe(tmp, arrlen(tmp), "%li/%li", current_index + 1, total_pages);
    gtk_label_set_text(self->entry_number_label, tmp);
}

static void refresh_dictname(UiManager *self, s8 dictname) {
    gtk_label_set_text(self->dictname_label, (char *)dictname.s);
}

static void refresh_title_with_entry(UiManager *self, Dictentry de) {
    _drop_(frees8) s8 title =
        concat(S("dictpopup - "), de.reading, S("【"), de.kanji, S("】from "), de.dictname);
    gtk_window_set_title(self->main_window, (char *)title.s);
}

static void set_frequency_to(UiManager *self, u32 freq) {
    char tmp[20] = {0};
    const char *freqstr = freq == 0 ? "" : tmp;
    if (freqstr == tmp)
        snprintf_safe(tmp, arrlen(tmp), "%i", freq);
    gtk_label_set_text(self->frequency_label, freqstr);
}

static void refresh_current_word_with_entry(UiManager *self, Word word) {
    _drop_(frees8) s8 txt = {0};
    if (word.kanji.len && word.reading.len) {
        if (s8equals(word.kanji, word.reading))
            txt = s8dup(word.reading.len ? word.reading : word.kanji);
        else
            txt = concat(word.reading, S("【"), word.kanji, S("】"));
    } else if (word.reading.len)
        txt = s8dup(word.reading);
    else if (word.kanji.len)
        txt = s8dup(word.kanji);
    else
        txt = s8dup(S(""));

    gtk_label_set_text(self->current_word_label, (char *)txt.s);
}

struct ui_refresh_args_s {
    UiManager *self;
    PageManager *pm;
};

typedef struct {
    UiManager *self;
    PageManager *pm;
} DelayedUpdatesArgs;

static void refresh_exists_dot(UiManager *self, PageManager *pm) {
    AnkiCollectionStatus status = pm_get_current_anki_status(pm);
    self->current_anki_status_color = map_ac_status_to_color(status);
    gtk_widget_queue_draw(self->anki_status_dot);
}

static void refresh_pron_button(UiManager *self, PageManager *pm) {
    Pronfile *pronfiles = pm_get_current_pronfiles(pm);
    if (pronfiles) {
        enable_button(self->btn_pronounce);
    } else {
        disable_button(self->btn_pronounce);
    }
}

static gboolean process_delayed_updates(void *voidarg) {
    DelayedUpdatesArgs *args = voidarg;
    UiManager *self = args->self;
    PageManager *pm = args->pm;

    refresh_exists_dot(self, pm);
    refresh_pron_button(self, pm);

    free(args);
    return G_SOURCE_REMOVE;
}

static void ui_queue_delayed_updates(UiManager *self, PageManager *pm) {
    DelayedUpdatesArgs *args = new (DelayedUpdatesArgs, 1);
    *args = (DelayedUpdatesArgs){.self = self, .pm = pm};

    gdk_threads_add_idle((GSourceFunc)process_delayed_updates, args);
}

static gboolean _ui_refresh_impl(gpointer data) {
    struct ui_refresh_args_s *args = (struct ui_refresh_args_s *)data;
    UiManager *self = args->self;
    PageManager *pm = args->pm;

    Dictentry de = pm_get_current_dictentry(pm);
    size_t current_index = pm_get_current_index(pm);
    size_t num_pages = pm_get_num_pages(pm);

    refresh_definition(self, de.definition);
    refresh_current_word_with_entry(self, dictentry_get_word(de));
    refresh_dictname(self, de.dictname);
    refresh_dictionary_number(self, current_index, num_pages);
    set_frequency_to(self, de.frequency);
    refresh_left_right_buttons(self, num_pages);
    refresh_title_with_entry(self, de);

    return G_SOURCE_REMOVE;
}

static void ui_queue_quick_updates(UiManager *self, PageManager *pm) {
    struct ui_refresh_args_s *args = new (struct ui_refresh_args_s, 1);
    args->self = self;
    args->pm = pm;

    gdk_threads_add_idle_full(G_PRIORITY_HIGH, (GSourceFunc)_ui_refresh_impl, args, NULL);
}

void _nonnull_ ui_refresh(UiManager *self, PageManager *pm) {
    ui_queue_quick_updates(self, pm);
    ui_queue_delayed_updates(self, pm);
}

void _nonnull_ ui_manager_set_error(UiManager *self, s8 message) {
    gtk_text_buffer_set_text(self->definition_textbuffer, (char *)message.s, (gint)message.len);
    disable_all_buttons(self);
}

static void pronounce_menu_item_clicked(GtkMenuItem *menuitem, gpointer user_data) {
    const char *filepath = g_object_get_data(G_OBJECT(menuitem), "filepath");
    play_audio_async(fromcstr_((char *)filepath));
}

static s8 create_label_for_pronfile(Pronfile pronfile) {
    s8 origin = pronfile.fileinfo.origin;
    s8 pitch_pattern = pronfile.fileinfo.pitch_pattern;
    s8 delimiter = fromcstr_(origin.len && pitch_pattern.len ? " - " : "");

    s8 label = concat(pitch_pattern, delimiter, origin);

    return label;
}

void show_pronunciation_button_right_click_menu(UiManager *self, Pronfile *pronfiles) {
    if (!pronfiles)
        return;

    GtkWidget *menu = gtk_menu_new();

    for (size_t i = 0; i < buf_size(pronfiles); i++) {
        _drop_(frees8) s8 label = create_label_for_pronfile(pronfiles[i]);
        GtkWidget *menu_item = gtk_menu_item_new_with_label((char *)label.s);

        char *filepath_copy = g_strdup((char *)pronfiles[i].filepath.s);
        g_object_set_data_full(G_OBJECT(menu_item), "filepath", filepath_copy, g_free);

        g_signal_connect(menu_item, "activate", G_CALLBACK(pronounce_menu_item_clicked), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), self->btn_pronounce, GDK_GRAVITY_SOUTH,
                             GDK_GRAVITY_NORTH_WEST, NULL);
}

/* -------------- EDIT LOOKUP DIALOG ---------------- */
typedef struct {
    void (*on_accept)(const char *new_lookup, void *user_data);
    void *user_data;
} EditLookupData;

static void on_edit_lookup_dialog_response(GtkDialog *dialog, gint response_id,
                                           gpointer user_data) {
    EditLookupData *data = (EditLookupData *)user_data;
    if (response_id == GTK_RESPONSE_ACCEPT) {
        GtkEntry *entry = GTK_ENTRY(g_object_get_data(G_OBJECT(dialog), "entry"));
        const char *new_lookup = gtk_entry_get_text(entry);

        data->on_accept(new_lookup, data->user_data);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    free(data);
}

static void on_entry_activate(GtkEntry *entry, GtkDialog *dialog) {
    gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
}

void ui_manager_show_edit_lookup_dialog(UiManager *self, const char *current_lookup,
                                        void (*on_accept)(const char *new_lookup, void *user_data),
                                        void *user_data) {
    GtkWidget *dialog =
        gtk_dialog_new_with_buttons("Edit Lookup String", GTK_WINDOW(self->main_window),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "Cancel",
                                    GTK_RESPONSE_CANCEL, "Apply", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(content_area), entry);

    gtk_entry_set_text(GTK_ENTRY(entry), current_lookup);

    g_object_set_data(G_OBJECT(dialog), "entry", entry);

    EditLookupData *data = new (EditLookupData, 1);
    data->on_accept = on_accept;
    data->user_data = user_data;

    g_signal_connect(dialog, "response", G_CALLBACK(on_edit_lookup_dialog_response), data);
    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), dialog);

    gtk_widget_show_all(dialog);
}

/* -------------- ANKI BUTTON RIGHT CLICK MENU ---------------- */
void ui_manager_show_anki_button_right_click_menu(
    UiManager *self, void (*on_clipboard_definition)(GtkMenuItem *self, gpointer user_data),
    gpointer user_data) {
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *menu_item = gtk_menu_item_new_with_label("Add with clipboard content as definition");
    g_signal_connect(menu_item, "activate", G_CALLBACK(on_clipboard_definition), user_data);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), self->btn_add_to_anki, GDK_GRAVITY_SOUTH,
                             GDK_GRAVITY_NORTH_WEST, NULL);
}