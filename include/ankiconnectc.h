#ifndef ANKICONNECTC_H
#define ANKICONNECTC_H

#include "ankiconnectc/send_request.h"
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
} AnkiCard;

void ankicard_free(AnkiCard *ac);

bool ac_check_connection(void);
s8 *ac_get_decks(char **error);
char **ac_get_notetypes(char **error);
char **ac_get_fields_for_notetype(const char *notetype, char **error);

typedef enum {
    AC_ERROR,
    AC_EXISTS,
    AC_DOES_NOT_EXIST,
    AC_EXISTS_SUSPENDED,
    AC_EXISTS_NEW
} AnkiCollectionStatus;
AnkiCollectionStatus ac_check_exists(char *deck, char *field, char *entry, char **error);

void ac_gui_search(char *deck, char *field, char *entry, char **error);
void _nonnull_ ac_addNote(AnkiCard ac, char **error);

/*
 * Stores the file at @path in the Anki media collection under the name
 * @filename. Doesn't overwrite existing files.
 */
void ac_store_file(char *filename, char *path, char **error);

// void ac_retval_free(retval_s ret);

#endif
