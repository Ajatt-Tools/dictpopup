#include "anki.h"

#include <ankiconnectc.h>
#include <objects/dict.h>
#include <platformdep/windowtitle.h>
#include <utils/messages.h>
#include <utils/str.h>

s8 focused_window_title = {0};

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

void anki_set_entry_of_field(AnkiFieldMapping *field_mapping, const char *field_name, AnkiFieldEntry entry) {
    // TODO
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
static s8 map_entry(s8 lookup, s8 sent, dictentry de, int i) {
    if (i < 0 || i > 9)
        err("Anki field mapping number %i is out of bounds.", i);

    if (i == 0) {
        return s8dup(S(""));
    } else if (i == 2) {
        return s8dup(sent);
    }
    else if (i == 3) {
        return add_bold_tags_around_word(sent, lookup);
    }
    else if (i == 4) {
        return s8dup(de.kanji);
    }
    else if (i == 5) {
        return s8dup(de.reading);
    }
    else if (i == 6) {
        return s8dup(de.definition);
    }
    else if (i == 7) {
        return create_furigana(de.kanji, de.reading);
    }
    else if (i == 8) {
        // TODO: Return windowtitle
        return s8dup(S(""));
    }
    else if (i == 9) {
        return s8dup(de.dictname);
    }
    else {
        err("Anki field mapping number %i is out of bounds.", i);
        return s8dup(S(""));
    }
}

static AnkiCard prepare_ankicard(s8 lookup, s8 sentence, dictentry de, AnkiConfig config) {
    char **fieldentries = new (char *, config.fieldmapping.num_fields);
    for (size_t i = 0; i < config.fieldmapping.num_fields; i++) {
        // TODO: fix
        fieldentries[i] = (char *)map_entry(lookup, sentence, de, config.fieldmapping.field_content[i]).s;
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

void create_ankicard(s8 lookup, s8 sentence, dictentry de, AnkiConfig config) {
    AnkiCard ac = prepare_ankicard(lookup, sentence, de, config);
    send_ankicard(ac);
    free_prepared_ankicard(ac);
}