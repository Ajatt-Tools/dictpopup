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

typedef struct {
  char *deck;
  char *notetype;
  bool copy_sentence;
  bool nuke_whitespace_in_sentence;
  const char * const* fieldnames;
  size_t numFields;
  u32 *fieldMapping;
} AnkiConfig;

void create_ankicard(s8 lookup, s8 sentence, dictentry de, AnkiConfig config);

#endif //ANKI_H
