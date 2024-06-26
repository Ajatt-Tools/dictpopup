#include <gtk/gtk.h>

#include "platformdep/audio.h"
#include "utils/util.h"
#include "messages.h"

void play_audio(s8 filepath) {
    _drop_(frees8) s8 cmd =
        concat(S("ffplay -nodisp -nostats -hide_banner -autoexit '"), filepath, S("'"));

    g_autoptr(GError) error = NULL;
    g_spawn_command_line_sync((char *)cmd.s, NULL, NULL, NULL, &error);
    if (error)
        err("Failed to play file %s: %s", (char *)filepath.s, error->message);
}