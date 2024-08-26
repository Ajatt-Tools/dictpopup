#include <glib/gi18n.h>
#include <locale.h>
#include "dictpopup-application.h"

int main(int argc, char *argv[]) {
    safe_focused_window_title();

    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    g_autoptr(GtkApplication) app = g_object_new(
        DP_TYPE_APPLICATION, "application_id", "com.github.Ajatt-Tools.dictpopup", "flags",
        G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_NON_UNIQUE,
        NULL);

    return g_application_run(G_APPLICATION(app), argc, argv);
}