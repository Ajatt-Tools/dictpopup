#include "dictpopup-application.h"
#include "utils/dp_profile.h"

int main(int argc, char *argv[]) {
    set_start_time_now();
    safe_focused_window_title();

    g_autoptr(GtkApplication) app = g_object_new(
        DP_TYPE_APPLICATION, "application_id", "com.github.Ajatt-Tools.dictpopup", "flags",
        G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_NON_UNIQUE | G_APPLICATION_DEFAULT_FLAGS,
        NULL);

    return g_application_run(G_APPLICATION(app), argc, argv);
}