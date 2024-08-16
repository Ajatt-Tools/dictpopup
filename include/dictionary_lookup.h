#ifndef DP_DICTPOPUP_H
#define DP_DICTPOPUP_H

#include "objects/dict.h"
#include "utils/util.h"

typedef struct {
    bool sort_dict_entries;
    const char *const *dict_sort_order;
    bool nuke_whitespace_of_lookup;
    bool fallback_to_mecab_conversion;
    bool lookup_longest_matching_prefix;
} DictpopupConfig;

/*
 * Looks up @lookup in the database and returns all corresponding dictentries in
 * a buffer (see include/buf.h)
 */
DictLookup _nonnull_ dictionary_lookup(s8 word, DictpopupConfig cfg);

#endif
