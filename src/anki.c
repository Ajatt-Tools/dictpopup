#include "anki.h"

#include <ankiconnectc.h>
#include <objects/dict.h>
#include <utils/messages.h>
#include <utils/str.h>

static s8 add_bold_tags_around_word(s8 sent, s8 word);
static s8 create_furigana(s8 kanji, s8 reading);

#define POSSIBLE_ENTRIES_S_NMEMB 9
typedef struct possible_entries_s {
    s8 sent;
    s8 boldsent;
    s8 dictkanji;
    s8 dictreading;
    s8 dictdefinition;
    s8 furigana;
    s8 windowname;
    s8 dictname;
} PossibleEntries;

static void _nonnull_ possible_entries_fill_with(PossibleEntries *pe, s8 lookup, s8 sent,
                                                 dictentry de) {
    pe->sent = sent;
    pe->boldsent = add_bold_tags_around_word(pe->sent, lookup);

    pe->dictdefinition = de.definition;
    pe->dictkanji = de.kanji.len > 0 ? de.kanji : de.reading;
    pe->dictreading = de.reading;
    pe->furigana = create_furigana(de.kanji, de.reading);
    pe->dictname = de.dictname;

    // TODO TODO
    // pe->windowname = focused_window_title;
}

// static void possible_entries_free(PossibleEntries pe) {
// frees8(&pe.boldsent);
// }

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
static s8 map_entry(PossibleEntries p, int i) {
    if (i < 0 || i > 9)
        err("Anki field mapping number %i is out of bounds.", i);

    return i == 0   ? S("")
           : i == 2 ? p.sent
           : i == 3 ? p.boldsent
           : i == 4 ? p.dictkanji
           : i == 5 ? p.dictreading
           : i == 6 ? p.dictdefinition
           : i == 7 ? p.furigana
           : i == 8 ? p.windowname
           : i == 9 ? p.dictname
                    : S("");
}

static AnkiCard prepare_ankicard(s8 lookup, s8 sentence, dictentry de, AnkiConfig config) {
    PossibleEntries possible_entries = {0};
    possible_entries_fill_with(&possible_entries, lookup, sentence, de);

    char **fieldentries = new (char *, config.numFields);
    for (size_t i = 0; i < config.numFields; i++) {
        fieldentries[i] = (char *)map_entry(possible_entries, config.fieldMapping[i]).s;
        // TODO: fix
    }

    return (AnkiCard){.deck = config.deck,
                      .notetype = config.notetype,
                      .num_fields = config.numFields,
                      .fieldnames = (char **)config.fieldnames,
                      .fieldentries = fieldentries};
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
    free(ac.fieldentries); // TODO
}