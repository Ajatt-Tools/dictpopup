#include "util.h"

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

typedef struct dictpopup_s {
    possible_entries_s pe;
} dictpopup_s;

dictpopup_s dictpopup_init(int argc, char **argv);

/*
 * Looks up @lookup in the database and returns all corresponding dictentries in
 * a buffer (see include/buf.h)
 */
dictentry *create_dictionary(dictpopup_s d[static 1]);

void create_ankicard(dictpopup_s d, dictentry de);
