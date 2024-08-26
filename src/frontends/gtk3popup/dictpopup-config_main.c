#include "dp-preferences-window.h"
#include <gtk/gtk.h>

static void activate(GtkApplication *app, gpointer user_data) {
    DpSettings *settings = dp_settings_new();
    GtkWidget *preferences_window =
        g_object_new(DP_TYPE_PREFERENCES_WINDOW, "settings", settings, NULL);
    gtk_window_set_application(GTK_WINDOW(preferences_window), app);
    gtk_widget_show_all(preferences_window);
}

int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    app =
        gtk_application_new("com.github.Ajatt-Tools.dictpopup-settings", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}