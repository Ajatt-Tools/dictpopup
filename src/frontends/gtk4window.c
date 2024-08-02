#include "dictpopup.h"
#include "utils.h"
#include "utils/util.h"
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

static GtkTextTag *link_tag = NULL;
GtkTextBuffer *dictBuffer = NULL;

static void fill_buffer_with_lookup(char *lookup_cstr) {
    s8 lookup = fromcstr_(lookup_cstr);
    dictentry *dict = create_dictionary(&lookup);

    gtk_text_buffer_set_text(dictBuffer, "", -1);
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(dictBuffer, &iter);
    for (isize i = 0; i < MIN(dictlen(dict), 2); i++) {
        if (i)
            putchar('\n');
        s8 def = dict[i].definition;
        gtk_text_buffer_insert(dictBuffer, &iter, (char *)def.s, def.len);
    }
}

static void insert_link(GtkTextBuffer *buffer, GtkTextIter *iter, const char *text) {
    gtk_text_buffer_insert_with_tags_by_name(buffer, iter, text, -1, "link", NULL);
}

static void character_clicked(GtkWidget *text_view, GtkTextIter *iter) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    char *lookup = gtk_text_buffer_get_text(buffer, iter, &end, true);
    fill_buffer_with_lookup(lookup);
}

static gboolean key_pressed(GtkEventController *controller, guint keyval, guint keycode,
                            GdkModifierType modifiers, GtkWidget *text_view) {
    GtkTextIter iter;
    GtkTextBuffer *buffer;

    switch (keyval) {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
            gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
            character_clicked(text_view, &iter);
            break;

        default:
            break;
    }

    return GDK_EVENT_PROPAGATE;
}

static void set_cursor_if_appropriate(GtkTextView *text_view, int x, int y);

static void released_cb(GtkGestureClick *gesture, guint n_press, double x, double y,
                        GtkWidget *text_view) {
    GtkTextIter start, end, iter;
    GtkTextBuffer *buffer;
    int tx, ty;

    if (gtk_gesture_single_get_button(GTK_GESTURE_SINGLE(gesture)) > 1)
        return;

    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), GTK_TEXT_WINDOW_WIDGET, x, y,
                                          &tx, &ty);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    /* we shouldn't follow a link if the user has selected something */
    gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
    if (gtk_text_iter_get_offset(&start) != gtk_text_iter_get_offset(&end))
        return;

    if (gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(text_view), &iter, tx, ty))
        character_clicked(text_view, &iter);
}

static void motion_cb(GtkEventControllerMotion *controller, double x, double y,
                      GtkTextView *text_view) {
    int tx, ty;

    gtk_text_view_window_to_buffer_coords(text_view, GTK_TEXT_WINDOW_WIDGET, x, y, &tx, &ty);

    set_cursor_if_appropriate(text_view, tx, ty);
}

static gboolean hovering_over_link = FALSE;

static void set_cursor_if_appropriate(GtkTextView *text_view, int x, int y) {
    gboolean hovering = FALSE;

    GtkTextIter iter;
    if (gtk_text_view_get_iter_at_location(text_view, &iter, x, y)) {
        if (gtk_text_iter_has_tag(&iter, link_tag)) {
            hovering = TRUE;
        }
    }

    if (hovering != hovering_over_link) {
        hovering_over_link = hovering;

        if (hovering_over_link)
            gtk_widget_set_cursor_from_name(GTK_WIDGET(text_view), "pointer");
        else
            gtk_widget_set_cursor_from_name(GTK_WIDGET(text_view), "text");
    }
}

static GtkWidget *create_definition_text_view(void) {
    GtkTextView *def_tw = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(def_tw, FALSE);
    gtk_text_view_set_wrap_mode(def_tw, GTK_WRAP_CHAR);
    gtk_text_view_set_cursor_visible(def_tw, FALSE);

    gtk_text_view_set_top_margin(def_tw, 5);
    gtk_text_view_set_bottom_margin(def_tw, 5);
    gtk_text_view_set_left_margin(def_tw, 5);
    gtk_text_view_set_right_margin(def_tw, 5);

    return GTK_WIDGET(def_tw);
}

static GtkWidget *create_lookup_text_view(void) {
    GtkWidget *lookup_tw = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(lookup_tw), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(lookup_tw), GTK_WRAP_WORD);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(lookup_tw), FALSE);

    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(lookup_tw), 20);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(lookup_tw), 20);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(lookup_tw), 20);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(lookup_tw), 20);
    gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(lookup_tw), 10);
    return lookup_tw;
}

static void activate(GtkApplication *app, char *lookup_cstr) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "dictwindow");
    g_object_add_weak_pointer(G_OBJECT(window), (gpointer *)&window);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_vbox);

    GtkWidget *lookup_tw = create_lookup_text_view();

    GtkEventController *controller = gtk_event_controller_key_new();
    g_signal_connect(controller, "key-pressed", G_CALLBACK(key_pressed), lookup_tw);
    gtk_widget_add_controller(lookup_tw, controller);

    controller = GTK_EVENT_CONTROLLER(gtk_gesture_click_new());
    g_signal_connect(controller, "released", G_CALLBACK(released_cb), lookup_tw);
    gtk_widget_add_controller(lookup_tw, controller);

    controller = gtk_event_controller_motion_new();
    g_signal_connect(controller, "motion", G_CALLBACK(motion_cb), lookup_tw);
    gtk_widget_add_controller(lookup_tw, controller);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(lookup_tw));
    gtk_text_buffer_set_enable_undo(buffer, TRUE);

    link_tag = gtk_text_buffer_create_tag(buffer, "link", "foreground", "blue", "underline",
                                          PANGO_UNDERLINE_SINGLE, "size-points", (gdouble)20, NULL);

    gtk_box_append(GTK_BOX(main_vbox), lookup_tw);

    GtkWidget *def_tw = create_definition_text_view();
    dictBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(def_tw));

    GtkWidget *sw = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), def_tw);
    gtk_box_append(GTK_BOX(main_vbox), sw);

    gtk_widget_set_vexpand(sw, TRUE);
    gtk_widget_set_valign(sw, GTK_ALIGN_FILL);

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
    insert_link(buffer, &iter, lookup_cstr);

    fill_buffer_with_lookup(lookup_cstr);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("Need to provide a lookup string.");
        return EXIT_FAILURE;
    }
    dictpopup_init();

    char *lookup = argv[1];

    GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), lookup);
    int status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);

    return status;
}
