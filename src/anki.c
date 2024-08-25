#include "anki.h"

#include <ankiconnectc.h>
#include <objects/dict.h>
#include <platformdep/windowtitle.h>
#include <utils/messages.h>
#include <utils/str.h>

s8 focused_window_title = S("");

void safe_focused_window_title(void) {
    focused_window_title = get_windowname();
}

AnkiFieldEntry anki_get_entry_of_field(AnkiFieldMapping field_mapping, const char *field_name) {
    for (u32 i = 0; i < field_mapping.num_fields; i++) {
        if (strcmp(field_mapping.field_names[i], field_name) == 0) {
            return field_mapping.field_content[i];
        }
    }
    return DP_ANKI_EMPTY;
}

static int get_index_in(char const *str, size_t size, char *arr[size]) {
    if (str && arr) {
        for (size_t i = 0; i < size; i++) {
            if (strcmp(str, arr[i]) == 0)
                return i;
        }
    }
    return -1;
}

static void field_mapping_add_new(AnkiFieldMapping *mapping, char *field_name,
                                  AnkiFieldEntry field_entry) {
    char **new_field_names = new (char *, mapping->num_fields + 1);
    for (u32 i = 0; i < mapping->num_fields; i++) {
        new_field_names[i] = mapping->field_names[i];
    }
    new_field_names[mapping->num_fields] = field_name;

    AnkiFieldEntry *new_field_entries = new (AnkiFieldEntry, mapping->num_fields + 1);
    for (u32 i = 0; i < mapping->num_fields; i++) {
        new_field_entries[i] = mapping->field_content[i];
    }
    new_field_entries[mapping->num_fields] = field_entry;

    mapping->field_names = new_field_names;
    mapping->field_content = new_field_entries;
    mapping->num_fields++;
}

void anki_set_entry_of_field(AnkiFieldMapping *field_mapping, char *field_name,
                             AnkiFieldEntry entry) {
    int ind = get_index_in(field_name, field_mapping->num_fields, field_mapping->field_names);

    if (ind >= 0)
        field_mapping->field_content[ind] = entry;
    else
        field_mapping_add_new(field_mapping, field_name, entry);
}

const char *anki_field_entry_to_str(AnkiFieldEntry entry) {
    switch (entry) {
        case DP_ANKI_EMPTY:
            return "Empty";
        case DP_ANKI_LOOKUP_STRING:
            return "Lookup String";
        case DP_ANKI_COPIED_SENTENCE:
            return "Copied Sentence";
        case DP_ANKI_BOLD_COPIED_SENTENCE:
            return "Copied Sentence (Bold Target)";
        case DP_ANKI_DICTIONARY_KANJI:
            return "Dictionary Kanji";
        case DP_ANKI_DICTIONARY_READING:
            return "Dictionary Reading";
        case DP_ANKI_DICTIONARY_DEFINITION:
            return "Dictionary Definition";
        case DP_ANKI_FURIGANA:
            return "Furigana";
        case DP_ANKI_FOCUSED_WINDOW_NAME:
            return "Focused Window Name";
        default:
            return "Unknown";
    }
}

static s8 add_bold_tags_around_word(s8 sent, s8 word) {
    return enclose_word_in_s8_with(sent, word, S("<b>"), S("</b>"));
}

static s8 create_furigana(s8 kanji, s8 reading) {
    return !kanji.len && !reading.len ? s8dup(S(""))
           : !kanji.len               ? s8dup(reading)
           : !reading.len             ? s8dup(kanji)
                                      : concat(kanji, S("["), reading, S("]"));
}

// TODO: Improve maintainability?
static s8 map_entry(s8 lookup, s8 sent, Dictentry de, AnkiFieldEntry i) {
    switch (i) {
        case DP_ANKI_EMPTY:
            return s8dup(S(""));
        case DP_ANKI_LOOKUP_STRING:
            return s8dup(lookup);
        case DP_ANKI_COPIED_SENTENCE:
            return s8dup(sent);
        case DP_ANKI_BOLD_COPIED_SENTENCE:
            return add_bold_tags_around_word(sent, lookup);
        case DP_ANKI_DICTIONARY_KANJI:
            return s8dup(de.kanji);
        case DP_ANKI_DICTIONARY_READING:
            return s8dup(de.reading);
        case DP_ANKI_DICTIONARY_DEFINITION:
            return s8dup(de.definition);
        case DP_ANKI_FURIGANA:
            return create_furigana(de.kanji, de.reading);
        case DP_ANKI_FOCUSED_WINDOW_NAME:
            return s8dup(focused_window_title);
        default:
            err("Encountered unknown field mapping number %i", i);
            return s8dup(S(""));
    }
}

static AnkiCard prepare_ankicard(s8 lookup, s8 sentence, Dictentry de, AnkiConfig config) {
    char **fieldentries = new (char *, config.fieldmapping.num_fields);
    for (size_t i = 0; i < config.fieldmapping.num_fields; i++) {
        // TODO: fix
        fieldentries[i] =
            (char *)map_entry(lookup, sentence, de, config.fieldmapping.field_content[i]).s;
    }

    return (AnkiCard){.deck = config.deck,
                      .notetype = config.notetype,
                      .num_fields = config.fieldmapping.num_fields,
                      .fieldnames = (char **)config.fieldmapping.field_names,
                      .fieldentries = fieldentries};
}

static void free_prepared_ankicard(AnkiCard ac) {
    // TODO
    for (size_t i = 0; i < ac.num_fields; i++)
        free(ac.fieldentries[i]);
    free(ac.fieldentries);
}

static void send_ankicard(AnkiCard ac) {
    char *error = NULL;
    ac_addNote(ac, &error);
    if (error) {
        err("Error adding card: %s", error);
        free(error);
    } else
        msg("Successfully added card.");
}

void create_ankicard(s8 lookup, s8 sentence, Dictentry de, AnkiConfig config) {
    AnkiCard ac = prepare_ankicard(lookup, sentence, de, config);
    send_ankicard(ac);
    free_prepared_ankicard(ac);
}