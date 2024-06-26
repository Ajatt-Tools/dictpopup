#ifndef DP_DICTPOPUP_H
#define DP_DICTPOPUP_H

#include "utils/util.h"

#include <settings.h>

/*
 * Should only be called once and as early as possible
 */
void dictpopup_init(void);

/*
 * Looks up @lookup in the database and returns all corresponding dictentries in
 * a buffer (see include/buf.h)
 */
dictentry *_nonnull_ create_dictionary(s8 *word);

void create_ankicard(s8 lookup, dictentry de, Config config);

#define POSSIBLE_ENTRIES_S_NMEMB 9
typedef struct possible_entries_s {
    s8 copiedsent;
    s8 boldsent;
    s8 dictkanji;
    s8 dictreading;
    s8 dictdefinition;
    s8 furigana;
    s8 windowname;
    s8 dictname;
} possible_entries_s;

struct dictpopup_s {
    possible_entries_s pe;
};

#endif
