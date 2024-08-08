



struct _KgxPreferencesWindow {
  AdwPreferencesDialog  parent_instance;

  DpSettings           *settings;
  GBindingGroup        *settings_binds;

  GtkWidget            *audible_bell;
  GtkWidget            *visual_bell;
  GtkWidget            *use_system_font;
  GtkWidget            *custom_font;
};

G_DEFINE_TYPE (DpPreferencesWindow, dp_preferences_window, ADW_TYPE_PREFERENCES_DIALOG)


static void
dp_preferences_window_dispose (GObject *object)
{
  DpPreferencesWindow *self = KGX_PREFERENCES_WINDOW (object);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (kgx_preferences_window_parent_class)->dispose (object);
}
