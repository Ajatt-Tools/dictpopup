#include "dp-settings.h"

#include <anki.h>
#include <gio/gio.h>

struct _DpSettings {
    GObject parent_instance;

    GSettings *settings;

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
    PROP_NUKE_WHITESPACE_LOOKUP,
    PROP_MECAB_CONVERSION,
    PROP_SUBSTRING_SEARCH,
    PROP_SORT_ORDER,
    PROP_ANKI_DECK,
    PROP_ANKI_NOTETYPE,
    PROP_ANKI_SEARCH_FIELD,
    N_PROPERTIES
};

static GParamSpec *pspecs[N_PROPERTIES] = {
    NULL,
};

static void dp_settings_set_property(GObject *object, guint property_id, const GValue *value,
                                     GParamSpec *pspec) {
    DpSettings *self = DP_SETTINGS(object);

    switch (property_id) {
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
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void dp_settings_get_property(GObject *object, guint property_id, GValue *value,
                                     GParamSpec *pspec) {
    DpSettings *self = DP_SETTINGS(object);

    switch (property_id) {
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

    pspecs[PROP_NUKE_WHITESPACE_LOOKUP] = g_param_spec_boolean(
        "nuke-whitespace-lookup", "Nuke Whitespace Lookup",
        "Whether to remove whitespace from lookup strings", TRUE, G_PARAM_READWRITE);

    pspecs[PROP_MECAB_CONVERSION] = g_param_spec_boolean(
        "mecab-conversion", "MeCab Conversion", "Whether to use MeCab for hiragana conversion",
        FALSE, G_PARAM_READWRITE);

    pspecs[PROP_SUBSTRING_SEARCH] =
        g_param_spec_boolean("substring-search", "Substring Search",
                             "Whether to enable substring search", TRUE, G_PARAM_READWRITE);

    pspecs[PROP_SORT_ORDER] =
        g_param_spec_boxed("dict-sort-order", "Dictionary Sort Order",
                           "Order of dictionaries for sorting", G_TYPE_STRV, G_PARAM_READWRITE);

    pspecs[PROP_ANKI_DECK] = g_param_spec_string(
        "anki-deck", "Anki Deck", "Name of the Anki deck to use", NULL, G_PARAM_READWRITE);

    pspecs[PROP_ANKI_NOTETYPE] =
        g_param_spec_string("anki-notetype", "Anki Note Type", "Name of the Anki note type to use",
                            NULL, G_PARAM_READWRITE);

    pspecs[PROP_ANKI_SEARCH_FIELD] =
        g_param_spec_string("anki-search-field", "Anki Search Field",
                            "Name of the Anki field to search in", NULL, G_PARAM_READWRITE);

    g_object_class_install_properties(object_class, N_PROPERTIES, pspecs);
}

static void dp_settings_init(DpSettings *self) {
    self->settings = g_settings_new("com.github.Ajatt-Tools.dictpopup");

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
}

DpSettings *dp_settings_new(void) {
    return g_object_new(DP_TYPE_SETTINGS, NULL);
}

/* Getter / Setter */
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

void dp_settings_set_dict_sort_order(DpSettings *self, char **dict_sort_order) {
    g_return_if_fail(DP_IS_SETTINGS(self));
    // TODO: Just keep a copy in GSettings? (No local)
    g_strfreev(self->dict_sort_order);
    self->dict_sort_order = dict_sort_order;
    g_settings_set_strv(self->settings, "dict-sort-order", (const char *const *)dict_sort_order);
}

/* ------------ ANKI ------------------- */
gchar *dp_settings_get_anki_deck(DpSettings *self) {
    g_return_val_if_fail(DP_IS_SETTINGS(self), NULL);
    return self->anki_deck;
}

void dp_settings_set_anki_notetype(DpSettings *self, char *new_anki_notetype) {
    g_return_if_fail(DP_IS_SETTINGS(self));

    if (strcmp(new_anki_notetype, self->anki_notetype) == 0)
        return;

    g_clear_pointer(&self->anki_notetype, g_free);
    self->anki_notetype = new_anki_notetype;

    g_object_notify_by_pspec(G_OBJECT(self), pspecs[PROP_ANKI_NOTETYPE]);
}

gchar *dp_settings_get_anki_notetype(DpSettings *self) {
    g_return_val_if_fail(DP_IS_SETTINGS(self), NULL);
    return self->anki_notetype;
}

gchar *dp_settings_get_anki_search_field(DpSettings *self) {
    g_return_val_if_fail(DP_IS_SETTINGS(self), NULL);
    return self->anki_search_field;
}

AnkiConfig dp_settings_get_anki_settings(DpSettings *self) {
    g_return_val_if_fail(DP_IS_SETTINGS(self), (AnkiConfig){0});

    return (AnkiConfig){.deck = self->anki_deck,
                        .notetype = self->anki_notetype,
                        .fieldmapping = dp_settings_get_anki_field_mappings(self)};
}

void dp_settings_set_anki_field_mappings(DpSettings *self, AnkiFieldMapping field_mapping) {
    g_settings_set_strv(self->settings, "anki-field-names",
                        (const char *const *)field_mapping.field_names);

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("ai"));
    for (gsize i = 0; i < field_mapping.num_fields; i++) {
        g_variant_builder_add(&builder, "i", (gint32)field_mapping.field_content[i]);
    }
    g_settings_set_value(self->settings, "anki-field-entries", g_variant_builder_end(&builder));
}

AnkiFieldMapping dp_settings_get_anki_field_mappings(DpSettings *self) {
    AnkiFieldMapping field_mapping = {0};
    field_mapping.field_names = g_settings_get_strv(self->settings, "anki-field-names");

    GVariant *variant = g_settings_get_value(self->settings, "anki-field-entries");
    field_mapping.num_fields = g_variant_n_children(variant);
    field_mapping.field_content = g_new(AnkiFieldEntry, field_mapping.num_fields);

    for (gsize i = 0; i < field_mapping.num_fields; i++) {
        gint32 value;
        g_variant_get_child(variant, i, "i", &value);
        field_mapping.field_content[i] = (AnkiFieldEntry)value;
    }

    g_variant_unref(variant);

    return field_mapping;
}