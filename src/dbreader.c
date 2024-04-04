#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "settings.h" // for dbpath

#include "lmdb.h"
#include "util.h"

#include "messages.h"
#define MDB_CHECK(call)                                              \
	if ((rc = call) != MDB_SUCCESS) {                            \
	    fatal("Database error: %s", mdb_strerror(rc));           \
	}
static int rc;

static MDB_env *env = 0;
static MDB_dbi dbi1 = 0;
static MDB_dbi dbi2 = 0;
static MDB_dbi dbi3 = 0;
static MDB_txn *txn = 0;

void
open_database(void)
{
    MDB_CHECK(mdb_env_create(&env));
    mdb_env_set_maxdbs(env, 3);

    if (!cfg.general.dbpth)
      fatal("Path to the database file is NULL! This is a bug.");
    MDB_CHECK(mdb_env_open(env, cfg.general.dbpth, MDB_RDONLY | MDB_NOLOCK | MDB_NORDAHEAD, 0664));
    MDB_CHECK(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));

    MDB_CHECK(mdb_dbi_open(txn, "db1", MDB_DUPSORT | MDB_DUPFIXED, &dbi1));
    MDB_CHECK(mdb_dbi_open(txn, "db2", MDB_INTEGERKEY, &dbi2));
    MDB_CHECK(mdb_dbi_open(txn, "db3", 0, &dbi3));
}

void
close_database(void)
{
    mdb_txn_abort(txn);
    mdb_dbi_close(env, dbi1);
    mdb_dbi_close(env, dbi2);
    mdb_dbi_close(env, dbi3);
    mdb_env_close(env);

    txn = 0;
    dbi1 = 0;
    dbi2 = 0;
    dbi3 = 0;
    env = 0;
}

/* 
 * Returns: Data associated to id from second database. 
 * Data is valid until closure of db and should not be freed.
 * WARNING: returned data is not null-terminated!
 */ 
s8
db_getdata(u32 id)
{
	MDB_val key = (MDB_val){ .mv_data = &id, .mv_size = sizeof(id) };
	MDB_val data = {0};
	MDB_CHECK(mdb_get(txn, dbi2, &key, &data));
	return (s8){ .s = data.mv_data, .len = data.mv_size  };
}

// valid until closure of db
u32*
db_getids(s8 key, size_t len[static 1])
{
    MDB_val key_mdb = (MDB_val) { .mv_data = key.s, .mv_size = (size_t)key.len };
    MDB_val val_mdb;

    MDB_cursor *cursor;
    MDB_CHECK(mdb_cursor_open(txn, dbi1, &cursor));
    if ((rc = mdb_cursor_get(cursor, &key_mdb, &val_mdb, MDB_SET)) == MDB_NOTFOUND)
	return NULL;
    MDB_CHECK(rc);
    MDB_CHECK(mdb_cursor_get(cursor, &key_mdb, &val_mdb, MDB_GET_MULTIPLE)); // Reads up to a page, i.e. max 1024 entries
    mdb_cursor_close(cursor);

    *len = val_mdb.mv_size / sizeof(u32);
    return (u32*)val_mdb.mv_data;
}

int
db_getfreq(s8 key)
{
    MDB_val key_m = (MDB_val) { .mv_data = key.s, .mv_size = (size_t)key.len };
    MDB_val val_m = { 0 };

    if ((rc = mdb_get(txn, dbi3, &key_m, &val_m)) == MDB_NOTFOUND)
	return -1;
    MDB_CHECK(rc);

    // FIXME?
    int val = 0;
    val |= ((byte*)val_m.mv_data)[0];
    val |= ((byte*)val_m.mv_data)[1] << 8;
    val |= ((byte*)val_m.mv_data)[2] << 16;
    val |= ((byte*)val_m.mv_data)[3] << 24;
    return val;
}
