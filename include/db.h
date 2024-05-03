#ifndef DP_DB_H
#define DP_DB_H

#include <stdbool.h>

#include <lmdb.h>

#include "util.h"

// A (not so) opaque struct
typedef struct database_s database_t;

database_t _nonnull_ db_open(char *dbpath, bool readonly);
void _nonnull_ db_close(database_t *db);

void _nonnull_ db_put_dictent(database_t *db, s8 headword, dictentry de);
void _nonnull_ db_get_dictents(database_t *db, s8 headword, dictentry *dict[static 1]);

void _nonnull_ db_put_freq(database_t *db, s8 word, s8 reading, u32 freq);
int _nonnull_ db_get_freq(database_t *db, s8 word, s8 reading);


/*
 * Checks if there exists a database in the provided path
 */
i32 db_check_exists(s8 dbpath);


struct database_s {
    MDB_env *env;
    MDB_dbi dbi1;
    MDB_dbi dbi2;
    MDB_dbi dbi3;
    MDB_txn *txn;
    stringbuilder_s lastdatastr;
    u32 last_id;
    bool readonly;
};

#endif
