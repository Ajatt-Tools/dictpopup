

static void activate(GtkApplication *app, gpointer user_data) {
    dictpopup_t *dictdata = (dictpopup_t *)user_data;
}

int main(int argc, char *argv[]) {
    dictpopup_t d = dictpopup_init(argc, argv);

    pthread_t thread;
    pthread_create(&thread, NULL, fill_dictionary, &d);
    pthread_detach(thread);

    GtkApplication *app =
        gtk_application_new("com.github.Ajatt-Tools.dictpopup", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &d);
    g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
}
