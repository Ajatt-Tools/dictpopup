#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <lmdb.h>

#include "database.h"
#include "util.h"

#define MDB_CHECK(call)                                                                            \
    do {                                                                                           \
        if ((rc = call) != MDB_SUCCESS) {                                                          \
            printf("Error: %s\n at %s (%d)\n", mdb_strerror(rc), __FILE__, __LINE__);              \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)

database opendb(const char *path, bool readonly) {
    int rc;
    database db = {0};
    MDB_CHECK(mdb_env_create(&db.env));
    MDB_CHECK(mdb_env_set_maxdbs(db.env, 2));

    db.readonly = readonly;

    if (db.readonly)
        MDB_CHECK(mdb_env_open(db.env, path, MDB_RDONLY | MDB_NOLOCK | MDB_NORDAHEAD, 0664));
    else {
        unsigned int mapsize = 2147483648; // 2 Gb max
        MDB_CHECK(mdb_env_set_mapsize(db.env, mapsize));

        MDB_CHECK(mdb_env_open(db.env, path, 0, 0664));
        MDB_CHECK(mdb_txn_begin(db.env, NULL, 0, &db.txn));
        MDB_CHECK(mdb_dbi_open(db.txn, "dbi1", MDB_DUPSORT | MDB_CREATE, &db.dbi1));
        MDB_CHECK(mdb_dbi_open(db.txn, "dbi2", MDB_CREATE, &db.dbi2));

        db.lastval = sb_init(200);
    }

    return db;
}

void closedb(database db) {
    if (!db.readonly) {
        int rc;
        MDB_CHECK(mdb_txn_commit(db.txn));
        mdb_dbi_close(db.env, db.dbi1);
        mdb_dbi_close(db.env, db.dbi2);
    }
    mdb_env_close(db.env);
}

/*
 * headword -> file db
 */
void addtodb1(database db, s8 key, s8 val) {
    int rc;
    /* msg("Adding key: %.*s with value %.*s", (int)key.len, (char*)key.s,
     * (int)val.len, (char*)val.s); */
    MDB_val mdb_key = {.mv_data = key.s, .mv_size = (size_t)key.len};
    MDB_val mdb_val = {.mv_data = val.s, .mv_size = (size_t)val.len};

    rc = mdb_put(db.txn, db.dbi1, &mdb_key, &mdb_val, MDB_NOOVERWRITE);
    if (rc == MDB_KEYEXIST) {
        if (s8equals(sb_gets8(db.lastval), key)) {
            mdb_val = (MDB_val){.mv_data = val.s, .mv_size = (size_t)val.len};
            rc = mdb_put(db.txn, db.dbi1, &mdb_key, &mdb_val, 0);
        }

        if (rc != MDB_KEYEXIST)
            MDB_CHECK(rc);
    } else
        MDB_CHECK(rc);

    if (rc == MDB_SUCCESS)
        sb_set(&db.lastval, key);
}

/*
 * file -> fileinfo db
 */
void addtodb2(database db, s8 key, s8 val) {
    int rc;
    MDB_val mdb_key = {.mv_data = key.s, .mv_size = (size_t)key.len};
    MDB_val mdb_val = {.mv_data = val.s, .mv_size = (size_t)val.len};
    MDB_CHECK(mdb_put(db.txn, db.dbi2, &mdb_key, &mdb_val, MDB_NOOVERWRITE));
}

s8 getfromdb2(database db, s8 key) {
    int rc;
    MDB_CHECK(mdb_txn_begin(db.env, NULL, MDB_RDONLY, &db.txn));
    MDB_CHECK(mdb_dbi_open(db.txn, "dbi2", 0, &db.dbi2));

    MDB_val key_m = (MDB_val){.mv_data = key.s, .mv_size = (size_t)key.len};
    MDB_val val_m = {0};

    if ((rc = mdb_get(db.txn, db.dbi2, &key_m, &val_m)) == MDB_NOTFOUND)
        return (s8){0};
    MDB_CHECK(rc);

    s8 ret = s8dup((s8){.s = val_m.mv_data, .len = val_m.mv_size});

    mdb_txn_abort(db.txn);
    return ret;
}

s8 *getfiles(database db, s8 key) {
    int rc;
    MDB_CHECK(mdb_txn_begin(db.env, NULL, MDB_RDONLY, &db.txn));
    MDB_CHECK(mdb_dbi_open(db.txn, "dbi1", MDB_DUPSORT, &db.dbi1));

    MDB_val key_m = (MDB_val){.mv_data = key.s, .mv_size = (size_t)key.len};
    MDB_val val_m = {0};

    MDB_cursor *cursor;
    MDB_CHECK(mdb_cursor_open(db.txn, db.dbi1, &cursor));

    s8 *ret = 0;
    bool first = true;
    while ((rc = mdb_cursor_get(cursor, &key_m, &val_m, first ? MDB_SET_KEY : MDB_NEXT_DUP)) == 0) {
        s8 val = s8dup((s8){.s = val_m.mv_data, .len = val_m.mv_size});
        buf_push(ret, val);

        first = false;
    }
    if (rc != MDB_NOTFOUND)
        MDB_CHECK(rc);

    mdb_cursor_close(cursor);
    mdb_txn_abort(db.txn);
    return ret;
}
