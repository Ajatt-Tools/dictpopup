#include <glib/gi18n.h>
#include <stdatomic.h>

#include "ankiconnectc.h"
#include "db.h"
#include "dictpopup_create.h"
#include "jppron/jppron.h"

#include "dp-preferences-window.h"

static const char *generate_index_str = N_("Generate Index");
static const char *stop_str = N_("Stop");

struct _DpPreferencesWindow {
    GtkWindow parent_instance;

    DpSettings *settings;

    GtkWidget *dictionaries_path_chooser;
    GtkWidget *dictpopup_create_button;
    GThread *dictpopup_create_thread;
    atomic_bool cancel_dictpopup_create;

    GtkWidget *nuke_whitespace_switch;
    GtkWidget *mecab_conversion_switch;
    GtkWidget *substring_search_switch;
    GtkWidget *dict_order_listbox;

    GtkWidget *anki_notetype_combo;
    GtkWidget *anki_fields_list;

    GtkWidget *pronounce_on_startup_switch;

    GtkWidget *pronunciation_path_chooser;
    GtkWidget *pron_index_generate_button;
    GThread *pron_index_generate_thread;
    atomic_bool cancel_pron_index_generate;
};

G_DEFINE_TYPE(DpPreferencesWindow, dp_preferences_window, GTK_TYPE_WINDOW)

enum { PROP_0, PROP_SETTINGS, LAST_PROP };
static GParamSpec *pspecs[LAST_PROP] = {
    NULL,
};

static GtkTargetEntry drag_entries[] = {{"GTK_LIST_BOX_ROW", GTK_TARGET_SAME_APP, 0}};
static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                                  GtkSelectionData *selection_data, guint info, guint32 time,
                                  gpointer user_data);
static void dp_preferences_window_update_dict_order(DpPreferencesWindow *self);
static void dp_preferences_window_save_dict_order(DpPreferencesWindow *self);
static void populate_anki_notetypes(DpPreferencesWindow *self);
static void on_notetype_changed(GtkComboBox *combo_box, gpointer user_data);
static void update_anki_fields(DpPreferencesWindow *self);

/* ------------ CALLBACKS ---------------- */
static void on_preferences_close_button_clicked(GtkButton *button, DpPreferencesWindow *self) {
    gtk_widget_destroy(GTK_WIDGET(self));
}

static void restore_generate_index_button(GtkWidget *button) {
    gtk_button_set_label(GTK_BUTTON(button), _(generate_index_str));

    GtkStyleContext *context = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(context, "suggested-action");
    gtk_style_context_remove_class(context, "destructive-action");
}

static void change_to_stop_button(GtkWidget *button) {
    gtk_button_set_label(GTK_BUTTON(button), _(stop_str));

    GtkStyleContext *context = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(context, "destructive-action");
    gtk_style_context_remove_class(context, "suggested-action");
}

static void cancel_pron_index_generation(DpPreferencesWindow *self) {
    atomic_store(&self->cancel_pron_index_generate, true);
    restore_generate_index_button(self->dictpopup_create_button);
}

static gboolean pron_index_generation_thread_finished(gpointer user_data) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(user_data);

    self->pron_index_generate_thread = NULL;
    gtk_button_set_label(GTK_BUTTON(self->pron_index_generate_button), _(generate_index_str));

    return G_SOURCE_REMOVE;
}

static gpointer pron_index_generation_thread_func(gpointer user_data) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(user_data);
    char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self->pronunciation_path_chooser));

    if (path)
        jppron_create_index(path, &self->cancel_pron_index_generate);

    g_free(path);

    gdk_threads_add_idle(pron_index_generation_thread_finished, self);

    return NULL;
}

static void start_pron_index_generation(DpPreferencesWindow *self) {
    atomic_store(&self->cancel_pron_index_generate, false);
    self->pron_index_generate_thread =
        g_thread_new("pron_index_generation_thread", pron_index_generation_thread_func, self);
    change_to_stop_button(self->pron_index_generate_button);
}

static void on_pron_index_generate_button_clicked(GtkButton *button, DpPreferencesWindow *self) {
    char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self->pronunciation_path_chooser));

    if (self->pron_index_generate_thread) {
        cancel_pron_index_generation(self);
    } else {
        start_pron_index_generation(self);
    }

    g_free(path);
}

/* --------- dictpopup db generation ---------- */

static bool overwrite_dictpopup_db_dialog(void *voidarg) {
    DpPreferencesWindow *pref_window = voidarg;
    gboolean overwrite = FALSE;

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(pref_window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        _("A database already exists. Do you want to overwrite it?"));
    gtk_window_set_title(GTK_WINDOW(dialog), _("Overwrite Existing Database?"));

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_YES) {
        overwrite = TRUE;
    }

    gtk_widget_destroy(dialog);

    return overwrite;
}

static void cancel_dictpopup_create(DpPreferencesWindow *self) {
    atomic_store(&self->cancel_dictpopup_create, true);
    restore_generate_index_button(self->dictpopup_create_button);
}

static gboolean dictpopup_create_thread_finished(gpointer user_data) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(user_data);

    self->dictpopup_create_thread = NULL;
    gtk_button_set_label(GTK_BUTTON(self->dictpopup_create_button), _(generate_index_str));

    dp_preferences_window_update_dict_order(self);

    return G_SOURCE_REMOVE;
}

static gpointer dictpopup_create_thread_func(gpointer user_data) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(user_data);
    char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self->dictionaries_path_chooser));

    if (path)
        dictpopup_create(path, overwrite_dictpopup_db_dialog, self, &self->cancel_dictpopup_create);

    g_free(path);

    gdk_threads_add_idle(dictpopup_create_thread_finished, self);

    return NULL;
}

static void start_dictpopup_create(DpPreferencesWindow *self) {
    atomic_store(&self->cancel_dictpopup_create, false);
    self->dictpopup_create_thread =
        g_thread_new("create_thread", dictpopup_create_thread_func, self);
    change_to_stop_button(self->dictpopup_create_button);
}

static void on_dictpopup_create_button_clicked(GtkButton *button, DpPreferencesWindow *self) {
    if (self->dictpopup_create_thread) {
        cancel_dictpopup_create(self);
    } else {
        start_dictpopup_create(self);
    }
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->keyval == GDK_KEY_Escape) {
        gtk_widget_destroy(widget);
        return TRUE;
    }
    return FALSE;
} /* ----------------------------------------- */

static void dp_preferences_window_dispose(GObject *object) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(object);

    if (self->dictpopup_create_thread) {
        atomic_store(&self->cancel_dictpopup_create, true);
        g_thread_join(self->dictpopup_create_thread);
    }

    if (self->pron_index_generate_thread) {
        atomic_store(&self->cancel_pron_index_generate, true);
        g_thread_join(self->pron_index_generate_thread);
    }

    g_clear_object(&self->settings);

    G_OBJECT_CLASS(dp_preferences_window_parent_class)->dispose(object);
}

static void dp_preferences_window_set_property(GObject *object, guint property_id,
                                               const GValue *value, GParamSpec *pspec) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(object);

    switch (property_id) {
        case PROP_SETTINGS:
            if (g_set_object(&self->settings, g_value_get_object(value))) {
                g_object_notify_by_pspec(G_OBJECT(self), pspecs[PROP_SETTINGS]);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void dp_preferences_window_get_property(GObject *object, guint property_id, GValue *value,
                                               GParamSpec *pspec) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(object);

    switch (property_id) {
        case PROP_SETTINGS:
            g_value_set_object(value, self->settings);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void dp_preferences_window_constructed(GObject *object) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(object);

    g_object_bind_property(self->settings, "nuke-whitespace-lookup", self->nuke_whitespace_switch,
                           "active", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
    g_object_bind_property(self->settings, "mecab-conversion", self->mecab_conversion_switch,
                           "active", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
    g_object_bind_property(self->settings, "substring-search", self->substring_search_switch,
                           "active", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

    g_object_bind_property(self->settings, "pronounce-on-startup",
                           self->pronounce_on_startup_switch, "active",
                           G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

    dp_preferences_window_update_dict_order(self);
    populate_anki_notetypes(self);
    g_signal_connect(self->anki_notetype_combo, "changed", G_CALLBACK(on_notetype_changed), self);
    update_anki_fields(self);
}

static void dp_preferences_window_class_init(DpPreferencesWindowClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->dispose = dp_preferences_window_dispose;
    object_class->set_property = dp_preferences_window_set_property;
    object_class->get_property = dp_preferences_window_get_property;
    object_class->constructed = dp_preferences_window_constructed;

    pspecs[PROP_SETTINGS] = g_param_spec_object(
        "settings", NULL, NULL, DP_TYPE_SETTINGS,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties(object_class, LAST_PROP, pspecs);

    gtk_widget_class_set_template_from_resource(
        widget_class, "/com/github/Ajatt-Tools/dictpopup/dp-preferences-window.ui");

    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow,
                                         dictionaries_path_chooser);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow,
                                         dictpopup_create_button);

    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow, nuke_whitespace_switch);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow,
                                         mecab_conversion_switch);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow,
                                         substring_search_switch);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow, dict_order_listbox);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow, anki_notetype_combo);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow, anki_fields_list);

    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow,
                                         pronounce_on_startup_switch);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow,
                                         pronunciation_path_chooser);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow,
                                         pron_index_generate_button);

    /* CALLBACKS */
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                            on_preferences_close_button_clicked);
    gtk_widget_class_bind_template_callback(widget_class, on_pron_index_generate_button_clicked);
    gtk_widget_class_bind_template_callback(widget_class, on_dictpopup_create_button_clicked);
}

static void dp_preferences_window_init(DpPreferencesWindow *self) {
    gtk_widget_init_template(GTK_WIDGET(self));

    g_signal_connect(self, "key-press-event", G_CALLBACK(on_key_press), NULL);
}

/* -------- START NOTETYPE ----------- */
static void populate_anki_notetypes(DpPreferencesWindow *self) {
    char *error = NULL;
    char **notetypes = ac_get_notetypes(&error);

    const char *current_notetype = dp_settings_get_anki_notetype(self->settings);
    int index_current_notetype = -1;

    if (notetypes) {
        for (int i = 0; notetypes[i] != NULL; i++) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(self->anki_notetype_combo),
                                           notetypes[i]);

            if (current_notetype && strcmp(current_notetype, notetypes[i]) == 0) {
                index_current_notetype = i;
            }

            free(notetypes[i]);
        }
        free(notetypes);
    }

    if (index_current_notetype >= 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(self->anki_notetype_combo), index_current_notetype);
    }
}

static void on_notetype_changed(GtkComboBox *combo_box, gpointer user_data) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(user_data);

    char *selected_notetype = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_box));

    dp_settings_set_anki_notetype(self->settings, selected_notetype);
    update_anki_fields(self);
}

/* ----------- START FIELD MAPPING ---------------- */
static void populate_anki_field_combo_box(GtkComboBoxText *combo) {
    gtk_combo_box_text_remove_all(combo);
    for (int i = 0; i < DP_ANKI_N_FIELD_ENTRIES; i++) {
        const char *entry_name = anki_field_entry_to_str(i);
        gtk_combo_box_text_append_text(combo, entry_name);
    }
}

static void on_field_entry_changed(GtkComboBox *combo, gpointer user_data) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(user_data);
    char *field_name = g_object_get_data(G_OBJECT(combo), "field_name");
    AnkiFieldEntry new_entry = gtk_combo_box_get_active(combo); // 危ないかも

    AnkiFieldMapping field_mapping = dp_settings_get_anki_field_mappings(self->settings);
    anki_set_entry_of_field(&field_mapping, field_name, new_entry);

    dp_settings_set_anki_field_mappings(self->settings, field_mapping);
}

static void clear_current_fields(DpPreferencesWindow *self) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(self->anki_fields_list));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}

static GtkWidget *create_anki_field_row(DpPreferencesWindow *self, char *field_name,
                                        AnkiFieldEntry current_entry) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *label = gtk_label_new(field_name);
    GtkWidget *combo = gtk_combo_box_text_new();

    gtk_widget_set_size_request(combo, 50, -1);

    gtk_container_add(GTK_CONTAINER(row), hbox);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
    gtk_widget_set_size_request(combo, 100, -1);

    g_object_set_data_full(G_OBJECT(combo), "field_name", g_strdup(field_name), g_free);
    populate_anki_field_combo_box(GTK_COMBO_BOX_TEXT(combo));

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), current_entry);

    g_signal_connect(combo, "changed", G_CALLBACK(on_field_entry_changed), self);
    return row;
}

static void update_anki_fields(DpPreferencesWindow *self) {
    const char *selected_notetype =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self->anki_notetype_combo));
    char *error = NULL;
    char **fields = ac_get_fields_for_notetype(selected_notetype, &error);

    clear_current_fields(self);

    AnkiFieldMapping current_field_mapping = dp_settings_get_anki_field_mappings(self->settings);

    if (fields) {
        for (int i = 0; fields[i] != NULL; i++) {
            AnkiFieldEntry current_entry =
                anki_get_entry_of_field(current_field_mapping, fields[i]);

            GtkWidget *row = create_anki_field_row(self, fields[i], current_entry);
            gtk_list_box_insert(GTK_LIST_BOX(self->anki_fields_list), row, -1);

            free(fields[i]);
        }
        free(fields);
    }

    gtk_widget_show_all(self->anki_fields_list);
}

/* ----------- START DRAG LIST ---------------- */
static void on_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer data) {
    GtkWidget *row;
    GtkAllocation alloc;
    cairo_surface_t *surface;
    cairo_t *cr;
    int x, y;
    double sx, sy;

    row = gtk_widget_get_ancestor(widget, GTK_TYPE_LIST_BOX_ROW);
    gtk_widget_get_allocation(row, &alloc);
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, alloc.width, alloc.height);
    cr = cairo_create(surface);

    gtk_style_context_add_class(gtk_widget_get_style_context(row), "drag-icon");
    gtk_widget_draw(row, cr);
    gtk_style_context_remove_class(gtk_widget_get_style_context(row), "drag-icon");

    gtk_widget_translate_coordinates(widget, row, 0, 0, &x, &y);
    cairo_surface_get_device_scale(surface, &sx, &sy);
    cairo_surface_set_device_offset(surface, -x * sx, -y * sy);
    gtk_drag_set_icon_surface(context, surface);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                                  GtkSelectionData *selection_data, guint info, guint32 time,
                                  gpointer data) {
    DpPreferencesWindow *app = DP_PREFERENCES_WINDOW(data);

    GtkWidget *target = widget;

    int pos = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(target));
    GtkWidget *row = (gpointer) * (gpointer *)gtk_selection_data_get_data(selection_data);
    GtkWidget *source = gtk_widget_get_ancestor(row, GTK_TYPE_LIST_BOX_ROW);

    if (source == target)
        return;

    g_object_ref(source);
    gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(source)), source);
    gtk_list_box_insert(GTK_LIST_BOX(gtk_widget_get_parent(target)), source, pos);
    g_object_unref(source);

    dp_preferences_window_save_dict_order(app);
}

static void on_drag_data_get(GtkWidget *widget, GdkDragContext *context,
                             GtkSelectionData *selection_data, guint info, guint time,
                             gpointer user_data) {
    gtk_selection_data_set(selection_data, gdk_atom_intern_static_string("GTK_LIST_BOX_ROW"), 32,
                           (const guchar *)&widget, sizeof(gpointer));
}

static GtkWidget *create_dict_row(DpPreferencesWindow *win, const char *dict_name) {
    GtkWidget *row = gtk_list_box_row_new();

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(row), box);

    GtkWidget *handle = gtk_event_box_new();
    GtkWidget *image =
        gtk_image_new_from_icon_name("format-justify-fill", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(handle), image);
    gtk_container_add(GTK_CONTAINER(box), handle);

    GtkWidget *label = gtk_label_new(dict_name);
    gtk_container_add_with_properties(GTK_CONTAINER(box), label, "expand", TRUE, NULL);

    gtk_drag_source_set(handle, GDK_BUTTON1_MASK, drag_entries, 1, GDK_ACTION_MOVE);
    g_signal_connect(handle, "drag-begin", G_CALLBACK(on_drag_begin), NULL);
    g_signal_connect(handle, "drag-data-get", G_CALLBACK(on_drag_data_get), NULL);

    gtk_drag_dest_set(row, GTK_DEST_DEFAULT_ALL, drag_entries, 1, GDK_ACTION_MOVE);
    g_signal_connect(row, "drag-data-received", G_CALLBACK(on_drag_data_received), win);

    return row;
}

static void destroy_widget(GtkWidget *widget, void *data) {
    gtk_widget_destroy(widget);
}

static void list_box_insert_msg(GtkListBox *list_box, const char *msg) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(msg);
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_list_box_insert(list_box, row, -1);
}

static GHashTable *hash_table_from_s8_buffer(s8Buf buf) {
    GHashTable *hash_table = g_hash_table_new(g_str_hash, g_str_equal);
    for (size_t i = 0; i < s8_buf_size(buf); i++) {
        g_hash_table_add(hash_table, (gpointer)buf[i].s);
    }
    return hash_table;
}

void dp_preferences_window_update_dict_order(DpPreferencesWindow *self) {
    gtk_container_foreach(GTK_CONTAINER(self->dict_order_listbox), (GtkCallback)destroy_widget,
                          NULL);

    database_t *db = db_open(true);
    if (!db) {
        list_box_insert_msg(GTK_LIST_BOX(self->dict_order_listbox), _("No database found"));
        return;
    }

    _drop_(s8_buf_free) s8Buf dict_names = db_get_dictnames(db);
    db_close(db);
    if (s8_buf_size(dict_names) == 0) {
        list_box_insert_msg(GTK_LIST_BOX(self->dict_order_listbox),
                            _("No dictionaries found in database"));
        return;
    }

    const gchar *const *current_order = dp_settings_get_dict_sort_order(self->settings);

    GHashTable *dict_names_table = hash_table_from_s8_buffer(dict_names);

    if (current_order) {
        for (int i = 0; current_order[i] != NULL; i++) {
            if (g_hash_table_contains(dict_names_table, current_order[i])) {
                GtkWidget *row = create_dict_row(self, current_order[i]);
                gtk_list_box_insert(GTK_LIST_BOX(self->dict_order_listbox), row, -1);
                g_hash_table_remove(dict_names_table, current_order[i]);
            }
        }
    }

    GHashTableIter iter;
    gpointer key;
    g_hash_table_iter_init(&iter, dict_names_table);
    while (g_hash_table_iter_next(&iter, &key, NULL)) {
        const char *dict_name = (const char *)key;
        GtkWidget *row = create_dict_row(self, dict_name);
        gtk_list_box_insert(GTK_LIST_BOX(self->dict_order_listbox), row, -1);
    }

    g_hash_table_destroy(dict_names_table);
    gtk_widget_show_all(GTK_WIDGET(self->dict_order_listbox));
}

static void dp_preferences_window_save_dict_order(DpPreferencesWindow *self) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(self->dict_order_listbox));
    GPtrArray *new_order = g_ptr_array_new();

    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        GtkWidget *box = gtk_bin_get_child(GTK_BIN(row));
        GtkWidget *label = gtk_container_get_children(GTK_CONTAINER(box))->next->data;
        const gchar *dict_name = gtk_label_get_text(GTK_LABEL(label));
        g_ptr_array_add(new_order, g_strdup(dict_name));
    }

    g_ptr_array_add(new_order, NULL); // NULL-terminate the array

    dp_settings_set_dict_sort_order(self->settings, (char **)new_order->pdata);

    g_ptr_array_free(new_order, TRUE);
    g_list_free(children);
}