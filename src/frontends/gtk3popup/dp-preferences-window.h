#ifndef DP_PREFERENCES_WINDOW_H
#define DP_PREFERENCES_WINDOW_H

#include "dp-settings.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DP_TYPE_PREFERENCES_WINDOW (dp_preferences_window_get_type())
G_DECLARE_FINAL_TYPE(DpPreferencesWindow, dp_preferences_window, DP, PREFERENCES_WINDOW, GtkWindow)

G_END_DECLS

#endif // DP_PREFERENCES_WINDOW_H