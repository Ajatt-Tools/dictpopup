#ifndef DP_DB_H
#define DP_DB_H

#include <stdbool.h>

#include <lmdb.h>

#include "util.h"

// An opaque struct
typedef struct database_s database_t;

typedef struct {
    s8 word;
    s8 reading;
    u32 frequency;
} frequency_entry;

database_t *_nonnull_ db_open(char *dbpath, bool readonly);
void _nonnull_ db_close(database_t *db);

void _nonnull_ db_put_dictent(database_t *db, s8 headword, dictentry de);
void _nonnull_ db_get_dictents(const database_t *db, s8 headword, dictentry *dict[static 1]);

void _nonnull_ db_put_freq(const database_t *db, frequency_entry fe);

/*
 * Checks if there exists a database in the provided path
 */
i32 db_check_exists(s8 dbpath);
void db_remove(s8 dbpath);

DEFINE_DROP_FUNC(database_t *, db_close)

#endif
