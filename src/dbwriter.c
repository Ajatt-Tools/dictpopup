#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lmdb.h>

#include "dbwriter.h"
#include "util.h"

#define MDB_CHECK(call)                                                                            \
    do {                                                                                           \
        if ((rc = call) != MDB_SUCCESS) {                                                          \
            printf("Error: %s\n at %s (%d)\n", mdb_strerror(rc), __FILE__, __LINE__);              \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)

MDB_env *env = 0;
MDB_dbi dbi1 = 0;
MDB_dbi dbi2 = 0;
MDB_dbi dbi3 = 0;
MDB_txn *txn = 0;

stringbuilder_s lastval = {0};
u32 entry_number = 0;

void opendb(const char *path) {
    int rc;
    MDB_CHECK(mdb_env_create(&env));
    mdb_env_set_maxdbs(env, 3);
    unsigned int mapsize = 2097152000; // 2Gb
    MDB_CHECK(mdb_env_set_mapsize(env, mapsize));

    MDB_CHECK(mdb_env_open(env, path, 0, 0664));
    MDB_CHECK(mdb_txn_begin(env, NULL, 0, &txn));

    MDB_CHECK(mdb_dbi_open(txn, "db1", MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE,
                           &dbi1)); // Can use MDB_GET_MULTIPLE and
                                    // MDB_NEXT_MULTIPLE
    MDB_CHECK(mdb_dbi_open(txn, "db2", MDB_INTEGERKEY | MDB_CREATE, &dbi2));
    MDB_CHECK(mdb_dbi_open(txn, "db3", MDB_CREATE, &dbi3)); // frequency

    lastval = sb_init(200);
}

void closedb(void) {
    sb_free(&lastval);

    printf("Total number of entries: %d\n", entry_number);

    int rc;
    MDB_CHECK(mdb_txn_commit(txn));
    mdb_dbi_close(env, dbi1);
    mdb_dbi_close(env, dbi2);
    mdb_dbi_close(env, dbi3);
    mdb_env_close(env);
}

void addtodb(s8 key, s8 val) {
    MDB_val key_mdb = {.mv_data = key.s, .mv_size = key.len};
    MDB_val id_mdb = {.mv_data = &entry_number, .mv_size = sizeof(entry_number)};

    if (!s8equals(val, sb_gets8(lastval))) {
        entry_number++;
        MDB_val val_mdb = {.mv_data = val.s, .mv_size = val.len};

        int rc;
        MDB_CHECK(mdb_put(txn, dbi2, &id_mdb, &val_mdb, MDB_NOOVERWRITE | MDB_APPEND));

        sb_set(&lastval, val);
    }

    // Add key with corresponding id
    mdb_put(txn, dbi1, &key_mdb, &id_mdb, MDB_NODUPDATA);
}

void db_add_int(s8 key, int val) {
    MDB_val key_mdb = {.mv_data = key.s, .mv_size = key.len};
    MDB_val val_mdb = {.mv_data = &val, .mv_size = sizeof(val)};

    int rc;
    MDB_CHECK(mdb_put(txn, dbi3, &key_mdb, &val_mdb, MDB_NODUPDATA));
}
