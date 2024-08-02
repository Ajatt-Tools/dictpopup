#ifndef DICTPOPUP_APP_H
#define DICTPOPUP_APP_H

#include "dict_state_manager.h"

#include <gtk/gtk.h>

#define DP_TYPE_APPLICATION (dp_application_get_type())
G_DECLARE_FINAL_TYPE(DpApplication, dp_application, DP, APPLICATION, GtkApplication)

struct _DpApplication {
    GtkApplication parent;

    GtkWindow *main_window;
    GtkWidget *settings_window;
    GtkWidget *settings_button;
    GtkLabel *dictionary_number_lbl;
    GtkTextBuffer *definition_textbuffer;
    GtkLabel *dictname_lbl;
    GtkLabel *frequency_lbl;
    GtkLabel *current_word_lbl;

    s8 lookup_word;

    DictManager dict_manager;
    GMutex dict_manager_mutex;
    GCancellable *current_cancellable;
};

void refresh_ui(DpApplication *app);

#endif // DICTPOPUP_APP_H
