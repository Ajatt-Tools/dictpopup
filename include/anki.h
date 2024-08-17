#ifndef ANKI_H
#define ANKI_H
#include <objects/dict.h>
#include <utils/str.h>

typedef enum {
    DP_ANKI_EMPTY = 0,
    DP_ANKI_LOOKUP_STRING,
    DP_ANKI_COPIED_SENTENCE,
    DP_ANKI_BOLD_COPIED_SENTENCE,
    DP_ANKI_DICTIONARY_KANJI,
    DP_ANKI_DICTIONARY_READING,
    DP_ANKI_DICTIONARY_DEFINITION,
    DP_ANKI_FURIGANA,
    DP_ANKI_FOCUSED_WINDOW_NAME,
    DP_ANKI_N_FIELD_ENTRIES,
} AnkiFieldEntry;

const char *anki_field_entry_to_str(AnkiFieldEntry entry);

typedef struct {
    char **field_names;
    AnkiFieldEntry *field_content;
    size_t num_fields;
} AnkiFieldMapping;

AnkiFieldEntry anki_get_entry_of_field(AnkiFieldMapping field_mapping, const char *field_name);
void anki_set_entry_of_field(AnkiFieldMapping *field_mapping, const char *field_name, AnkiFieldEntry entry);

typedef struct {
    char *deck;
    char *notetype;
    AnkiFieldMapping fieldmapping;
} AnkiConfig;

void create_ankicard(s8 lookup, s8 sentence, dictentry de, AnkiConfig config);

#endif // ANKI_H
