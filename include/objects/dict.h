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
isize dictLen(Dict dict);

// Sorts @dict in place
void dictSort(Dict dict, int (*dictentryComparer)(const dictentry *a, const dictentry *b));

void _nonnull_ dictionary_free(Dict *dict);
dictentry dictentry_at_index(Dict dict, isize index);
dictentry *pointer_to_entry_at(Dict dict, isize index);

#endif // DICT_H
