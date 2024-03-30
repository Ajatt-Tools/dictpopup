#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "lmdb.h"
#include "util.h"

int rc = 0;
#define MDB_CHECK(call)                                  \
	do {                                             \
	    if ((rc = call) != MDB_SUCCESS) {            \
		printf("Error: %s\n at %s (%d)\n", mdb_strerror(rc), __FILE__, __LINE__); \
		exit(EXIT_FAILURE);                      \
	    }                                            \
	} while (0)

MDB_env *env = 0;
MDB_dbi dbi1 = { 0 };
MDB_dbi dbi2 = { 0 };
MDB_txn *txn = 0;

s8 last_val = { 0 };
uint32_t entry_number = 0;

void
opendb(const char* path)
{
    MDB_CHECK(mdb_env_create(&env));
    mdb_env_set_maxdbs(env, 2);
    unsigned int mapsize = 2097152000;     // 2Gb
    MDB_CHECK(mdb_env_set_mapsize(env, mapsize));

    MDB_CHECK(mdb_env_open(env, path, 0, 0664));
    MDB_CHECK(mdb_txn_begin(env, NULL, 0, &txn));

    MDB_CHECK(mdb_dbi_open(txn, "db1", MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, &dbi1)); // Can use MDB_GET_MULTIPLE and MDB_NEXT_MULTIPLE
    MDB_CHECK(mdb_dbi_open(txn, "db2", MDB_INTEGERKEY | MDB_CREATE, &dbi2));
}

void
closedb(void)
{
    frees8(&last_val);

    MDB_CHECK(mdb_txn_commit(txn));
    mdb_dbi_close(env, dbi1);
    mdb_dbi_close(env, dbi2);
    mdb_env_close(env);
}

size_t
get_num_entries(void)
{
    //TODO: Implement
    return 0;
}

void
addtodb(s8 key, s8 val)
{
    MDB_val key_mdb = { .mv_data = key.s, .mv_size = key.len };
    MDB_val id_mdb = { .mv_data = &entry_number, .mv_size = sizeof(entry_number) };

    if (!s8equals(val, last_val))
    {
	entry_number++;

	MDB_val val_mdb = { .mv_data = val.s, .mv_size = val.len };
	MDB_CHECK(mdb_put(txn, dbi2, &id_mdb, &val_mdb, MDB_NOOVERWRITE | MDB_APPEND));

	frees8(&last_val);
	last_val = s8dup(val);
    }

    // Add key with corresponding id
    mdb_put(txn, dbi1, &key_mdb, &id_mdb, MDB_NODUPDATA);
}