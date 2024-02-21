#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "lmdb.h"
#include "util.h"

int rc;
#define MDB_CHECK(call) \
	if ((rc = call) != MDB_SUCCESS) { \
		printf("Error: %s\n", mdb_strerror(rc)); \
		exit(EXIT_FAILURE); \
	}

MDB_env *env;
MDB_dbi dbi1, dbi2;
MDB_txn *txn;

s8 last_val;
uint32_t entry_number = 0;

void
opendb(const char* path)
{
	MDB_CHECK(mdb_env_create(&env));
	mdb_env_set_maxdbs(env, 3);
	unsigned int mapsize = 2097152000; // 2Gb
	MDB_CHECK(mdb_env_set_mapsize(env, mapsize));

	MDB_CHECK(mdb_env_open(env, path, 0, 0664));
	MDB_CHECK(mdb_txn_begin(env, NULL, 0, &txn));

	MDB_CHECK(mdb_dbi_open(txn, "db1", MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, &dbi1)); // Can use MDB_GET_MULTIPLE and MDB_NEXT_MULTIPLE
	MDB_CHECK(mdb_dbi_open(txn, "db2", MDB_INTEGERKEY | MDB_CREATE, &dbi2));
}

void
closedb(void)
{
	MDB_CHECK(mdb_txn_commit(txn));
	mdb_dbi_close(env, dbi1);
	mdb_dbi_close(env, dbi2);
	mdb_env_close(env);

	printf("Number of distinct entries: %i\n", entry_number);
}

void
addtodb(unsigned char* key_str, ptrdiff_t keylen, unsigned char* value_str, ptrdiff_t vallen)
{
	MDB_val key_s = { .mv_data = key_str, .mv_size = keylen };
	MDB_val value_s = { .mv_data = value_str, .mv_size = vallen };
	MDB_val id_s = { .mv_data = &entry_number, .mv_size = sizeof(entry_number) };

	s8 cur_val = (s8){ .s = value_str, .len = vallen };
	if (!s8equals(cur_val, last_val))
	{
		entry_number++;
		// Add dictionary entry with entry_number as id
		MDB_CHECK(mdb_put(txn, dbi2, &id_s, &value_s, MDB_NOOVERWRITE | MDB_APPEND));

		free(last_val.s);
		last_val = s8dup(cur_val);
	}

	// Add key with corresponding id
	mdb_put(txn, dbi1, &key_s, &id_s, MDB_NODUPDATA);
}
