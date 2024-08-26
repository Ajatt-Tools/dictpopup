#include <gtk/gtk.h>

#include "platformdep/audio.h"
#include "utils/messages.h"
#include <utils/str.h>

static void child_watch_cb(GPid pid, gint status, gpointer user_data) {
    g_spawn_close_pid(pid);
}

void play_audio_async(s8 filepath) {
    _drop_(frees8) s8 cmd =
        concat(S("ffplay -nodisp -nostats -hide_banner -autoexit '"), filepath, S("'"));

    g_autoptr(GError) error = NULL;
    GPid child_pid;
    gchar **argv = NULL;

    if (!g_shell_parse_argv((char *)cmd.s, NULL, &argv, &error)) {
        err("Failed to parse command: %s", error->message);
        return;
    }

    if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, NULL,
                       NULL, &child_pid, &error)) {
        err("Failed to play file %s asynchronously: %s", (char *)filepath.s, error->message);
        g_strfreev(argv);
        return;
    }

    g_child_watch_add(child_pid, child_watch_cb, NULL);
    g_strfreev(argv);
}