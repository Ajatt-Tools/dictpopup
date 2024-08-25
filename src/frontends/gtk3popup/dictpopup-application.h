#ifndef DICTPOPUP_APP_H
#define DICTPOPUP_APP_H

#include "dp-preferences-window.h"
#include "dp-settings.h"
#include "dp_page_manager.h"
#include "ui_manager.h"
#include "jppron/jppron.h"

#include <gtk/gtk.h>

#define DP_TYPE_APPLICATION (dp_application_get_type())
G_DECLARE_FINAL_TYPE(DpApplication, dp_application, DP, APPLICATION, GtkApplication)

struct _DpApplication {
    GtkApplication parent;

    DpSettings *settings;

    DpPreferencesWindow *preferences_window;

    UiManager ui_manager;

    GMutex dict_data_mutex;
    s8 initial_lookup_str;
    s8 actual_lookup_str;
    PageManager page_manager;
};

void dp_play_current_pronunciation(DpApplication *self);

s8 dp_get_lookup_str(DpApplication *self);
Word dp_get_current_word(DpApplication *self);
_deallocator_(free_pronfile_buffer)
Pronfile *dp_get_current_pronfiles(DpApplication *self);
Dictentry dp_get_current_dictentry(DpApplication *self);

void dp_swap_dict_lookup(DpApplication *app, DictLookup new_dict_lookup);
void dp_swap_initial_lookup(DpApplication *app, s8 new_inital_lookup);

#endif // DICTPOPUP_APP_H
