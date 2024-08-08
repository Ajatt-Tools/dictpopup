#ifndef DICTPOPUP_APP_H
#define DICTPOPUP_APP_H

#include "dict_state_manager.h"
#include "dp-settings.h"

#include <gtk/gtk.h>

#define DP_TYPE_APPLICATION (dp_application_get_type())
G_DECLARE_FINAL_TYPE(DpApplication, dp_application, DP, APPLICATION, GtkApplication)

struct _DpApplication {
    GtkApplication parent;

    DpSettings *settings;

    GtkWindow *main_window;
    GtkWidget *settings_window;
    GtkWidget *settings_button;
    GtkLabel *dictionary_number_lbl;
    GtkTextBuffer *definition_textbuffer;
    GtkLabel *dictname_lbl;
    GtkLabel *frequency_lbl;
    GtkLabel *current_word_lbl;
    GtkWidget *anki_status_dot;

    s8 lookup_str;

    DictManager dict_manager; // Should never be accessed directly
    GMutex dict_manager_mutex;
};

s8 *dp_get_lookup_str(DpApplication *app);
Word *dp_get_copy_of_current_word(DpApplication *app);

void refresh_ui(DpApplication *app);
void dp_swap_dict_lookup(DpApplication *app, DictLookup new_dict_lookup);

#endif // DICTPOPUP_APP_H
