#ifndef DP_SETTINGS_H
#define DP_SETTINGS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DP_FONT_SCALE_DEFAULT 1.0

#define DP_TYPE_SETTINGS (dp_settings_get_type())
G_DECLARE_FINAL_TYPE (DpSettings, dp_settings, DP, SETTINGS, GObject)

DpSettings *dp_settings_new (void);

const gchar *dp_settings_get_database_path (DpSettings *self);

gboolean dp_settings_get_sort_entries (DpSettings *self);
const gchar * const *dp_settings_get_dict_sort_order (DpSettings *self);
gboolean dp_settings_get_nuke_whitespace_of_lookup (DpSettings *self);
gboolean dp_settings_get_mecab_conversion (DpSettings *self);
gboolean dp_settings_get_lookup_longest_matching_prefix (DpSettings *self);
const gchar * const *dp_settings_get_dict_sort_order (DpSettings *self);

const gchar *dp_settings_get_anki_deck (DpSettings *self);
const gchar *dp_settings_get_anki_notetype (DpSettings *self);
const gchar *dp_settings_get_anki_search_field (DpSettings *self);
const gchar * const *dp_settings_get_anki_fieldnames (DpSettings *self);
const gint *dp_settings_get_anki_fieldentries (DpSettings *self, gsize *length);

G_END_DECLS

#endif // DP_SETTINGS_H