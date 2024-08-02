#ifndef DP_DICTPOPUP_H
#define DP_DICTPOPUP_H

#include "objects/dict.h"
#include "utils/util.h"
#include <settings.h>

typedef dictentry *Dict;
/*
 * Should only be called once and as early as possible
 */
void dictpopup_init(void);

/*
 * Looks up @lookup in the database and returns all corresponding dictentries in
 * a buffer (see include/buf.h)
 */
DictLookup _nonnull_ dictionary_lookup(s8 word, Config config);

void create_ankicard(s8 lookup, s8 sentence, dictentry de, Config config);

#endif
