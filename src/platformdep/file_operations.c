#include <stdio.h>

#include <gtk/gtk.h>

#include "platformdep/file_operations.h"
#include "utils/messages.h"

void createdir(char *dirpath) {
    int ret = g_mkdir_with_parents(dirpath, 0777);
    die_on(ret != 0, "Creating directory '%s': %s", dirpath, strerror(errno));
}

bool check_file_exists(const char *fn) {
    return access(fn, R_OK) == 0;
}

const char *get_user_data_dir(void) {
    return g_get_user_data_dir();
}

void rem(char *filepath) {
    remove(filepath);
}

void file_copy_sync(const char *source_path, const char *dest_path) {
    g_autoptr(GFile) source = g_file_new_for_path(source_path);
    g_autoptr(GFile) dest = g_file_new_for_path(dest_path);

    g_autoptr(GError) error = NULL;
    g_file_copy(source, dest, G_FILE_COPY_NONE, NULL, NULL, NULL, &error);
    if (error)
        dbg("Error copying file: %s", error->message);
}

static void copy_ready_cb(GObject *source_object, GAsyncResult *res, gpointer data) {
    GError *error = NULL;
    g_file_copy_finish(G_FILE(source_object), res, &error);

    if (error)
        dbg("Error copying file: %s", error->message);
}

void file_copy_async(const char *source_path, const char *dest_path) {
    g_autoptr(GFile) source = g_file_new_for_path(source_path);
    g_autoptr(GFile) dest = g_file_new_for_path(dest_path);
    g_file_copy_async(source, dest, G_FILE_COPY_NONE, G_PRIORITY_LOW, NULL, NULL, NULL,
                      copy_ready_cb, NULL);
}