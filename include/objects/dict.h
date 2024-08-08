#ifndef DICT_H
#define DICT_H
#include "utils/str.h"
#include "utils/util.h"

typedef struct {
    s8 kanji;
    s8 reading;
} Word;

Word word_dup(Word word);
void word_ptr_free(Word *word);

typedef struct {
    s8 dictname;
    s8 kanji;
    s8 reading;
    s8 definition;
    u32 frequency;
    bool is_deinflection;
} dictentry;

dictentry dictentry_dup(dictentry de);
void _nonnull_ dictentry_free(dictentry de);
void dictentry_print(dictentry de);

typedef struct {
    dictentry *entries;
} Dict;

Dict newDict(void);
bool isEmpty(Dict dict);
void _nonnull_ dictionary_add(Dict *dict, dictentry de);
size_t num_of_dictentries(Dict dict);

// Sorts @dict in place
// Not thread safe
void dict_sort(Dict dict, const char* const* sort_order);

void dict_free(Dict dict);
dictentry dictentry_at_index(Dict dict, size_t index);

typedef struct {
    s8 lookup;
    Dict dict;
} DictLookup;

void dict_lookup_free(DictLookup *dl);

#endif // DICT_H
