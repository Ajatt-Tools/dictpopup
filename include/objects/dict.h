#ifndef DICT_H
#define DICT_H
#include "utils/str.h"
#include "utils/util.h"

typedef struct {
    s8 kanji;
    s8 reading;
} Word;

Word word_dup(Word word);
void word_free(Word word);
DEFINE_DROP_FUNC(Word, word_free)

typedef struct {
    s8 dictname;
    s8 kanji;
    s8 reading;
    s8 definition;
    u32 frequency;
    bool is_deinflection;
} Dictentry;

Dictentry dictentry_dup(Dictentry de);
void _nonnull_ dictentry_free(Dictentry de);
void dictentry_print(Dictentry de);

Word dictentry_get_word(Dictentry de);
Word dictentry_get_dup_word(Dictentry de);

typedef struct {
    Dictentry *entries;
} Dict;

Dict newDict(void);
bool isEmpty(Dict dict);
void _nonnull_ dictionary_add(Dict *dict, Dictentry de);
size_t num_entries(Dict dict);

// Sorts @dict in place
// Not thread safe
void dict_sort(Dict dict, const char *const *sort_order);

void dict_free(Dict dict, bool free_entries);
Dictentry dictentry_at_index(Dict dict, size_t index);

typedef struct {
    s8 lookup;
    Dict dict;
} DictLookup;

void dict_lookup_free(DictLookup *dl);

// For testing
bool dict_contains(Dict dict, s8 word);

#endif // DICT_H
