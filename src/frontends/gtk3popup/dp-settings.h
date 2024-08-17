#ifndef DP_SETTINGS_H
#define DP_SETTINGS_H

#include <anki.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DP_FONT_SCALE_DEFAULT 1.0

#define DP_TYPE_SETTINGS (dp_settings_get_type())
G_DECLARE_FINAL_TYPE(DpSettings, dp_settings, DP, SETTINGS, GObject)

DpSettings *dp_settings_new(void);

gboolean dp_settings_get_sort_entries(DpSettings *self);
gboolean dp_settings_get_nuke_whitespace_of_lookup(DpSettings *self);
gboolean dp_settings_get_mecab_conversion(DpSettings *self);
gboolean dp_settings_get_lookup_longest_matching_prefix(DpSettings *self);
const gchar *const *dp_settings_get_dict_sort_order(DpSettings *self);
void dp_settings_set_dict_sort_order(DpSettings *self, char **sort_order);


gchar *dp_settings_get_anki_deck(DpSettings *self);

void dp_settings_set_anki_notetype(DpSettings *self, char *new_anki_notetype);
gchar *dp_settings_get_anki_notetype(DpSettings *self);

gchar *dp_settings_get_anki_search_field(DpSettings *self);
AnkiConfig dp_settings_get_anki_settings(DpSettings *self);

void dp_settings_set_anki_field_mappings(DpSettings *self, AnkiFieldMapping field_mapping);
AnkiFieldMapping dp_settings_get_anki_field_mappings(DpSettings *self);

G_END_DECLS

#endif // DP_SETTINGS_H