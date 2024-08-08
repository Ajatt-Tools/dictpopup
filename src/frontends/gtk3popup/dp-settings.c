#include "dp-settings.h"

#include <anki.h>
#include <gio/gio.h>

struct _DpSettings {
    GObject parent_instance;

    GSettings *settings;

    gchar *database_path;
    gboolean sort_entries;
    gchar **dict_sort_order;
    gboolean nuke_whitespace_of_lookup;
    gboolean mecab_conversion;
    gboolean substring_search;

    gchar *anki_deck;
    gchar *anki_notetype;
    gchar *anki_search_field;
    gchar **anki_fieldnames;
    GArray *anki_fieldentries;
};

G_DEFINE_TYPE(DpSettings, dp_settings, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_DATABASE_PATH,
    PROP_SORT_ENTRIES,
    PROP_NUKE_WHITESPACE_LOOKUP,
    PROP_MECAB_CONVERSION,
    PROP_SUBSTRING_SEARCH,
    PROP_SORT_ORDER,
    PROP_ANKI_DECK,
    PROP_ANKI_NOTETYPE,
    PROP_ANKI_SEARCH_FIELD,
    PROP_ANKI_FIELDNAMES,
    PROP_ANKI_FIELDENTRIES,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {
    NULL,
};

static void dp_settings_set_property(GObject *object, guint property_id, const GValue *value,
                                     GParamSpec *pspec) {
    DpSettings *self = DP_SETTINGS(object);

    switch (property_id) {
        case PROP_DATABASE_PATH:
            g_free(self->database_path);
            self->database_path = g_value_dup_string(value);
            break;
        case PROP_SORT_ENTRIES:
            self->sort_entries = g_value_get_boolean(value);
            break;
        case PROP_NUKE_WHITESPACE_LOOKUP:
            self->nuke_whitespace_of_lookup = g_value_get_boolean(value);
            break;
        case PROP_MECAB_CONVERSION:
            self->mecab_conversion = g_value_get_boolean(value);
            break;
        case PROP_SUBSTRING_SEARCH:
            self->substring_search = g_value_get_boolean(value);
            break;
        case PROP_SORT_ORDER:
            self->dict_sort_order = g_value_dup_boxed(value);
            break;
        case PROP_ANKI_DECK:
            g_free(self->anki_deck);
            self->anki_deck = g_value_dup_string(value);
            break;
        case PROP_ANKI_NOTETYPE:
            g_free(self->anki_notetype);
            self->anki_notetype = g_value_dup_string(value);
            break;
        case PROP_ANKI_SEARCH_FIELD:
            g_free(self->anki_search_field);
            self->anki_search_field = g_value_dup_string(value);
            break;
        case PROP_ANKI_FIELDNAMES:
            g_strfreev(self->anki_fieldnames);
            self->anki_fieldnames = g_value_dup_boxed(value);
            break;
        case PROP_ANKI_FIELDENTRIES:
            if (self->anki_fieldentries)
                g_array_free(self->anki_fieldentries, TRUE);
            self->anki_fieldentries = g_array_ref(g_value_get_boxed(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void dp_settings_get_property(GObject *object, guint property_id, GValue *value,
                                     GParamSpec *pspec) {
    DpSettings *self = DP_SETTINGS(object);

    switch (property_id) {
        case PROP_DATABASE_PATH:
            g_value_set_string(value, self->database_path);
            break;
        case PROP_SORT_ENTRIES:
            g_value_set_boolean(value, self->sort_entries);
            break;
        case PROP_NUKE_WHITESPACE_LOOKUP:
            g_value_set_boolean(value, self->nuke_whitespace_of_lookup);
            break;
        case PROP_MECAB_CONVERSION:
            g_value_set_boolean(value, self->mecab_conversion);
            break;
        case PROP_SUBSTRING_SEARCH:
            g_value_set_boolean(value, self->substring_search);
            break;
        case PROP_SORT_ORDER:
            g_value_set_boxed(value, self->dict_sort_order);
            break;
        case PROP_ANKI_DECK:
            g_value_set_string(value, self->anki_deck);
            break;
        case PROP_ANKI_NOTETYPE:
            g_value_set_string(value, self->anki_notetype);
            break;
        case PROP_ANKI_SEARCH_FIELD:
            g_value_set_string(value, self->anki_search_field);
            break;
        case PROP_ANKI_FIELDNAMES:
            g_value_set_boxed(value, self->anki_fieldnames);
            break;
        case PROP_ANKI_FIELDENTRIES:
            g_value_set_boxed(value, self->anki_fieldentries);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void dp_settings_finalize(GObject *object) {
    DpSettings *self = DP_SETTINGS(object);

    g_strfreev(self->anki_fieldnames);
    if (self->anki_fieldentries)
        g_array_free(self->anki_fieldentries, TRUE);

    G_OBJECT_CLASS(dp_settings_parent_class)->finalize(object);
}

static void dp_settings_class_init(DpSettingsClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = dp_settings_set_property;
    object_class->get_property = dp_settings_get_property;
    object_class->finalize = dp_settings_finalize;

    obj_properties[PROP_DATABASE_PATH] =
        g_param_spec_string("database-path", "Database Path", "Path to the dictionary database",
                            NULL, G_PARAM_READWRITE);

    obj_properties[PROP_SORT_ENTRIES] =
        g_param_spec_boolean("sort-entries", "Sort Entries", "Whether to sort dictionary entries",
                             FALSE, G_PARAM_READWRITE);

    obj_properties[PROP_NUKE_WHITESPACE_LOOKUP] = g_param_spec_boolean(
        "nuke-whitespace-lookup", "Nuke Whitespace Lookup",
        "Whether to remove whitespace from lookup strings", TRUE, G_PARAM_READWRITE);

    obj_properties[PROP_MECAB_CONVERSION] = g_param_spec_boolean(
        "mecab-conversion", "MeCab Conversion", "Whether to use MeCab for hiragana conversion",
        FALSE, G_PARAM_READWRITE);

    obj_properties[PROP_SUBSTRING_SEARCH] =
        g_param_spec_boolean("substring-search", "Substring Search",
                             "Whether to enable substring search", TRUE, G_PARAM_READWRITE);

    obj_properties[PROP_SORT_ORDER] =
        g_param_spec_boxed("dict-sort-order", "Dictionary Sort Order",
                           "Order of dictionaries for sorting", G_TYPE_STRV, G_PARAM_READWRITE);

    obj_properties[PROP_ANKI_DECK] = g_param_spec_string(
        "anki-deck", "Anki Deck", "Name of the Anki deck to use", NULL, G_PARAM_READWRITE);

    obj_properties[PROP_ANKI_NOTETYPE] =
        g_param_spec_string("anki-notetype", "Anki Note Type", "Name of the Anki note type to use",
                            NULL, G_PARAM_READWRITE);

    obj_properties[PROP_ANKI_SEARCH_FIELD] =
        g_param_spec_string("anki-search-field", "Anki Search Field",
                            "Name of the Anki field to search in", NULL, G_PARAM_READWRITE);

    obj_properties[PROP_ANKI_FIELDNAMES] =
        g_param_spec_boxed("anki-fieldnames", "Anki Field Names", "Names of the Anki note fields",
                           G_TYPE_STRV, G_PARAM_READWRITE);

    obj_properties[PROP_ANKI_FIELDENTRIES] = g_param_spec_boxed(
        "anki-fieldentries", "Anki Field Entries",
        "Integer values corresponding to the Anki field entries", G_TYPE_ARRAY, G_PARAM_READWRITE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void dp_settings_init(DpSettings *self) {
    self->settings = g_settings_new("com.github.Ajatt-Tools.dictpopup");

    g_settings_bind(self->settings, "database-path", self, "database-path",
                    G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(self->settings, "sort-entries", self, "sort-entries", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(self->settings, "nuke-whitespace-lookup", self, "nuke-whitespace-lookup",
                    G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(self->settings, "mecab-conversion", self, "mecab-conversion",
                    G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(self->settings, "substring-search", self, "substring-search",
                    G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(self->settings, "dict-sort-order", self, "dict-sort-order",
                    G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(self->settings, "anki-deck", self, "anki-deck", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(self->settings, "anki-notetype", self, "anki-notetype",
                    G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(self->settings, "anki-search-field", self, "anki-search-field",
                    G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(self->settings, "anki-fieldnames", self, "anki-fieldnames",
                    G_SETTINGS_BIND_DEFAULT);

    self->anki_fieldentries = g_array_new (FALSE, FALSE, sizeof (AnkiFieldEntry));
    GVariant *variant = g_settings_get_value (self->settings, "anki-fieldentries");
    if (variant) {
        gsize length;
        const gint32 *array = g_variant_get_fixed_array (variant, &length, sizeof (gint32));
        for (gsize i = 0; i < length; i++) {
            if (array[i] >= 0 && array[i] < DP_ANKI_N_FIELD_ENTRIES) {
                AnkiFieldEntry field = (AnkiFieldEntry)array[i];
                g_array_append_val (self->anki_fieldentries,  field);
            }
            else {
                g_warning("Field entry No. %li is not a valid", i+1);
                AnkiFieldEntry empty_entry = DP_ANKI_EMPTY;
                g_array_append_val (self->anki_fieldentries,  empty_entry);
            }
        }
        g_variant_unref (variant);
    }
}

DpSettings *dp_settings_new(void) {
    return g_object_new(DP_TYPE_SETTINGS, NULL);
}

/* Getter / Setter */

// Getter functions
const gchar *dp_settings_get_database_path(DpSettings *self) {
    g_return_val_if_fail(DP_IS_SETTINGS(self), NULL);
    return self->database_path;
}

gboolean dp_settings_get_sort_entries(DpSettings *self) {
    return self->sort_entries;
}

gboolean dp_settings_get_nuke_whitespace_of_lookup(DpSettings *self) {
    return self->nuke_whitespace_of_lookup;
}

gboolean dp_settings_get_mecab_conversion(DpSettings *self) {
    return self->mecab_conversion;
}

gboolean dp_settings_get_lookup_longest_matching_prefix(DpSettings *self) {
    return self->substring_search;
}

const gchar *const *dp_settings_get_dict_sort_order(DpSettings *self) {
    g_return_val_if_fail(DP_IS_SETTINGS(self), NULL);
    return (const gchar *const *)self->dict_sort_order;
}

/* ------------ ANKI ------------------- */
const gchar *dp_settings_get_anki_deck(DpSettings *self) {
    g_return_val_if_fail(DP_IS_SETTINGS(self), NULL);
    return self->anki_deck;
}

const gchar *dp_settings_get_anki_notetype(DpSettings *self) {
    g_return_val_if_fail(DP_IS_SETTINGS(self), NULL);
    return self->anki_notetype;
}

const gchar *dp_settings_get_anki_search_field(DpSettings *self) {
    g_return_val_if_fail(DP_IS_SETTINGS(self), NULL);
    return self->anki_search_field;
}

const gchar * const *
dp_settings_get_anki_fieldnames (DpSettings *self)
{
    g_return_val_if_fail (DP_IS_SETTINGS (self), NULL);
    return (const gchar * const *)self->anki_fieldnames;
}

const gint *
dp_settings_get_anki_fieldentries (DpSettings *self, gsize *length)
{
    g_return_val_if_fail (DP_IS_SETTINGS (self), NULL);
    g_return_val_if_fail (length != NULL, NULL);

    *length = self->anki_fieldentries->len;
    return (const gint *)self->anki_fieldentries->data;
}