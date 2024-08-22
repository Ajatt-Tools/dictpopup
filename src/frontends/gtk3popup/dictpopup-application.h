#ifndef DICTPOPUP_APP_H
#define DICTPOPUP_APP_H

#include "dp-preferences-window.h"
#include "dp-settings.h"
#include "dp_page_manager.h"
#include "ui_manager.h"

#include <gtk/gtk.h>

#define DP_TYPE_APPLICATION (dp_application_get_type())
G_DECLARE_FINAL_TYPE(DpApplication, dp_application, DP, APPLICATION, GtkApplication)

struct _DpApplication {
    GtkApplication parent;

    DpSettings *settings;

    DpPreferencesWindow *preferences_window;

    s8 initial_lookup_str;
    s8 actual_lookup_str;

    PageManager page_manager;
    UiManager ui_manager;
};

s8 dp_get_lookup_str(DpApplication *app);
Word dp_get_copy_of_current_word(DpApplication *app);
void dp_show_page(DpApplication *self, PageData page, size_t num_pages);

void dp_swap_dict_lookup(DpApplication *app, DictLookup new_dict_lookup);
void dp_swap_initial_lookup(DpApplication *app, s8 new_inital_lookup);

void dp_application_show_preferences(DpApplication *self);

void enable_dictionary_buttons(DpApplication *app);
void set_no_dictentry_found(DpApplication *app);

#endif // DICTPOPUP_APP_H
