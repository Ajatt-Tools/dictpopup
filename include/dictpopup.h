#ifndef DP_DICTPOPUP_H
#define DP_DICTPOPUP_H

#include <settings.h>
#include "utils/util.h"
#include "objects/dict.h"

typedef dictentry* Dict;
/*
 * Should only be called once and as early as possible
 */
void dictpopup_init(void);

/*
 * Looks up @lookup in the database and returns all corresponding dictentries in
 * a buffer (see include/buf.h)
 */
Dict _nonnull_ create_dictionary(s8 *word, Config config);

void create_ankicard(s8 lookup, dictentry de, Config config);

#endif
