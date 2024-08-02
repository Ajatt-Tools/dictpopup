#include "objects/dict.h"
#include "utils/str.h"

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
    return NULL;
}

bool isEmpty(Dict dict) {
    return buf_size(dict) == 0;
}

void dictionary_add(dictentry **dict, dictentry de) {
    buf_push(*dict, de);
}

size_t dictLen(dictentry *dict) {
    return buf_size(dict);
}

void dictSort(Dict dict, int (*dictentryComparer)(const dictentry *a, const dictentry *b)) {
    if (!isEmpty(dict))
        qsort(dict, dictLen(dict), sizeof(dictentry),
              (int (*)(const void *, const void *))dictentryComparer);
}

void dictentry_free(dictentry de) {
    frees8(&de.dictname);
    frees8(&de.kanji);
    frees8(&de.reading);
    frees8(&de.definition);
}

void dict_free(Dict dict) {
    while (buf_size(dict) > 0)
        dictentry_free(buf_pop(dict));
    buf_free(dict);
}

dictentry dictentry_at_index(Dict dict, size_t index) {
    assert(index < buf_size(dict));
    return dict[index];
}

dictentry *pointer_to_entry_at(dictentry *dict, isize index) {
    assert(index >= 0 && (size_t)index < buf_size(dict));
    return dict + index;
}

void dict_lookup_free(DictLookup *dl) {
    dict_free(dl->dict);
    frees8(&dl->lookup);
    free(dl);
}