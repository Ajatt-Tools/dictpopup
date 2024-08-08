#include "objects/dict.h"
#include "utils/str.h"

#include <limits.h>
#include <string.h>

Word word_dup(Word word) {
    return (Word) { .kanji = s8dup(word.kanji), .reading = s8dup(word.reading)};
}

void word_ptr_free(Word *word) {
    frees8(&word->kanji);
    frees8(&word->reading);
    free(word);
}

dictentry dictentry_dup(dictentry de) {
    return (dictentry){.dictname = s8dup(de.dictname),
                       .kanji = s8dup(de.kanji),
                       .reading = s8dup(de.reading),
                       .definition = s8dup(de.definition)};
}

void dictentry_print(dictentry de) {
    printf("dictname: %s\n"
           "kanji: %s\n"
           "reading: %s\n"
           "definition: %s\n",
           (char *)de.dictname.s, (char *)de.kanji.s, (char *)de.reading.s,
           (char *)de.definition.s);
}

Dict newDict() {
    return (Dict){0};
}

bool isEmpty(Dict dict) {
    return buf_size(dict.entries) == 0;
}

void dictionary_add(Dict *dict, dictentry de) {
    buf_push(dict->entries, de);
}

size_t num_of_dictentries(Dict dict) {
    return buf_size(dict.entries);
}

static int indexof(char const *str, const char* const* arr) {
    if (str && arr) {
        for (int i = 0; arr[i]; i++) {
            if (strcmp(str, arr[i]) == 0)
                return i;
        }
    }
    return INT_MAX;
}

// qsort_r is unfortunately only available for GNU
static const char* const* SORT_ORDER = NULL;
/*
 * -1 means @a comes first
 */
static int _nonnull_ dictentry_comparer(const dictentry *a, const dictentry *b) {
    int inda, indb;

    // TODO: Make this less cryptic
    if (a->is_deinflection ^ b->is_deinflection) {
        inda = a->is_deinflection;
        indb = b->is_deinflection;
    } else if (s8equals(a->dictname, b->dictname)) {
        inda = a->frequency == 0 ? INT_MAX : a->frequency;
        indb = b->frequency == 0 ? INT_MAX : b->frequency;
    } else {
        inda = indexof((char *)a->dictname.s, SORT_ORDER);
        indb = indexof((char *)b->dictname.s, SORT_ORDER);
    }

    return inda < indb ? -1 : inda == indb ? 0 : 1;
}

void dict_sort(Dict dict, const char* const* sort_order) {
    SORT_ORDER = sort_order;
    if (!isEmpty(dict))
        qsort(dict.entries, num_of_dictentries(dict), sizeof(dictentry),
              (int (*)(const void *, const void *))dictentry_comparer);
    SORT_ORDER = 0;
}

void dictentry_free(dictentry de) {
    frees8(&de.dictname);
    frees8(&de.kanji);
    frees8(&de.reading);
    frees8(&de.definition);
}

void dict_free(Dict dict) {
    while (buf_size(dict.entries) > 0)
        dictentry_free(buf_pop(dict.entries));
    buf_free(dict.entries);
}

dictentry dictentry_at_index(Dict dict, size_t index) {
    assert(index < buf_size(dict.entries));
    return dict.entries[index];
}


void dict_lookup_free(DictLookup *dl) {
    dict_free(dl->dict);
    frees8(&dl->lookup);
    free(dl);
}