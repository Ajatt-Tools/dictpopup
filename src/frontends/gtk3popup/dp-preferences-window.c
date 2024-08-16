#include "dp-preferences-window.h"

#include <db.h>

struct _DpPreferencesWindow {
    GtkWindow parent_instance;

    DpSettings *settings;

    GtkWidget *sort_entries_switch;
    GtkWidget *nuke_whitespace_switch;
    GtkWidget *mecab_conversion_switch;
    GtkWidget *substring_search_switch;
    GtkWidget *dict_order_listbox;
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
void dp_preferences_window_populate_dict_order(DpPreferencesWindow *self);
void dp_preferences_window_save_dict_order(DpPreferencesWindow *self);

static void dp_preferences_window_dispose(GObject *object) {
    DpPreferencesWindow *self = DP_PREFERENCES_WINDOW(object);

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

    g_object_bind_property(self->settings, "sort-entries", self->sort_entries_switch, "active",
                           G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
    g_object_bind_property(self->settings, "nuke-whitespace-lookup", self->nuke_whitespace_switch,
                           "active", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
    g_object_bind_property(self->settings, "mecab-conversion", self->mecab_conversion_switch,
                           "active", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
    g_object_bind_property(self->settings, "substring-search", self->substring_search_switch,
                           "active", G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

    dp_preferences_window_populate_dict_order(self);
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

    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow, sort_entries_switch);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow, nuke_whitespace_switch);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow,
                                         mecab_conversion_switch);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow,
                                         substring_search_switch);
    gtk_widget_class_bind_template_child(widget_class, DpPreferencesWindow, dict_order_listbox);
}

static void dp_preferences_window_init(DpPreferencesWindow *self) {
    gtk_widget_init_template(GTK_WIDGET(self));
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
        gtk_image_new_from_icon_name("document-properties-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
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

void dp_preferences_window_populate_dict_order(DpPreferencesWindow *self) {
    gtk_container_foreach(GTK_CONTAINER(self->dict_order_listbox), (GtkCallback)destroy_widget,
                          NULL);

    const gchar *const *current_order = dp_settings_get_dict_sort_order(self->settings);
    assert(current_order);

    s8 dbpath = db_get_dbpath();
    database_t *db = db_open(dbpath, true);
    s8Buf dict_names = db_get_dictnames(db);
    db_close(db);

    GHashTable *order_hash = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; current_order[i] != NULL; i++) {
        g_hash_table_insert(order_hash, (void *)current_order[i], GINT_TO_POINTER(i));
    }

    for (int i = 0; current_order[i] != NULL; i++) {
        for (size_t j = 0; j < s8_buf_size(dict_names); j++) {
            if (strcmp(current_order[i], (char *)dict_names[j].s) == 0) {
                GtkWidget *row = create_dict_row(self, current_order[i]);
                gtk_list_box_insert(GTK_LIST_BOX(self->dict_order_listbox), row, -1);
                break;
            }
        }
    }

    // Add rows for any remaining dictionaries not in the current order
    for (size_t i = 0; i < s8_buf_size(dict_names); i++) {
        if (!g_hash_table_lookup(order_hash, (char *)dict_names[i].s)) {
            GtkWidget *row = create_dict_row(self, (char *)dict_names[i].s);
            gtk_list_box_insert(GTK_LIST_BOX(self->dict_order_listbox), row, -1);
        }
    }

    g_hash_table_destroy(order_hash);
    s8_buf_free(dict_names);
}

void dp_preferences_window_save_dict_order(DpPreferencesWindow *self) {
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