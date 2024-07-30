#ifndef DP_DB_H
#define DP_DB_H
#include "objects/dict.h"
#include "objects/freqentry.h"
#include "utils/util.h"
#include <stdbool.h>

// An opaque struct
typedef struct database_s database_t;

__attribute__((returns_nonnull)) database_t *_nonnull_ db_open(char *dbpath, bool readonly);
void _nonnull_ db_close(database_t *db);

void _nonnull_ db_put_dictent(database_t *db, dictentry de);
void _nonnull_ db_append_lookup(const database_t *db, s8 headword, Dict dict[static 1],
                                bool is_deinflection);

void _nonnull_ db_put_freq(const database_t *db, freqentry fe);

/*
 * Checks if there exists a database in the provided path
 */
bool db_check_exists(s8 dbpath);
void db_remove(s8 dbpath);

DEFINE_DROP_FUNC(database_t *, db_close)

#endif
