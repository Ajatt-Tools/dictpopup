#ifndef DICT_H
#define DICT_H
#include "utils/str.h"
#include "utils/util.h"

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

typedef dictentry *Dict;

Dict newDict(void);
bool isEmpty(Dict dict);
void _nonnull_ dictionary_add(Dict *dict, dictentry de);
size_t dictLen(Dict dict);

// Sorts @dict in place
void dictSort(Dict dict, int (*dictentryComparer)(const dictentry *a, const dictentry *b));

void dict_free(Dict dict);
dictentry dictentry_at_index(Dict dict, size_t index);
dictentry *pointer_to_entry_at(Dict dict, isize index);

typedef struct {
    s8 lookup;
    Dict dict;
} DictLookup;

void dict_lookup_free(DictLookup *dl);

#endif // DICT_H
