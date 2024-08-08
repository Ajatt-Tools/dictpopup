#ifndef ANKICONNECTC_H
#define ANKICONNECTC_H

#include "utils/util.h"
#include <stdbool.h>
#include <utils/str.h>

typedef struct {
    char *deck;
    char *notetype;

    size_t num_fields;
    char **fieldnames;
    char **fieldentries;
    char **tags;
} ankicard;

void ankicard_free(ankicard *ac);

bool ac_check_connection(void);
s8 *ac_get_decks(char **error);
s8 *ac_get_notetypes(char **error);

typedef enum { AC_ERROR, AC_EXISTS, AC_DOES_NOT_EXIST, AC_EXISTS_SUSPENDED, AC_EXISTS_NEW } AnkiCollectionStatus;
AnkiCollectionStatus ac_check_exists(const char *deck, const char *field, const char *entry, char **error);

void ac_gui_search(const char *deck, const char *field, const char *entry, char **error);
void _nonnull_ ac_addNote(ankicard ac, char **error);

/*
 * Stores the file at @path in the Anki media collection under the name
 * @filename. Doesn't overwrite existing files.
 */
void ac_store_file(const char *filename, const char *path, char **error);

#endif
