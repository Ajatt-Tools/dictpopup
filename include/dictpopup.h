#ifndef DP_DICTPOPUP_H
#define DP_DICTPOPUP_H

#include "util.h"

// Opaque type
typedef struct dictpopup_s dictpopup_t;

dictpopup_t dictpopup_init(int argc, char **argv);

/*
 * Looks up @lookup in the database and returns all corresponding dictentries in
 * a buffer (see include/buf.h)
 */
dictentry *_nonnull_ create_dictionary(dictpopup_t *d);

void create_ankicard(dictpopup_t d, dictentry de);

#define POSSIBLE_ENTRIES_S_NMEMB 9
typedef struct possible_entries_s {
    s8 lookup;
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
