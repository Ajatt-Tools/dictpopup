#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <lmdb.h>

int rc;
#define MDB_CHECK(call) \
	if ((rc = call) != MDB_SUCCESS) { \
		printf("Error: %s\n", mdb_strerror(rc)); \
		exit(EXIT_FAILURE); \
	}

MDB_env *env;
MDB_dbi dbi1, dbi2;
MDB_txn *txn;

MDB_env *env_tmp;
MDB_txn *txn_tmp;
MDB_dbi dbi_tmp;

unsigned int entry_number = 0;

void
open_db(const char* path)
{
	MDB_CHECK(mdb_env_create(&env));
	mdb_env_set_maxdbs(env, 3);
	unsigned int mapsize = 2097152000;
	MDB_CHECK(mdb_env_set_mapsize(env, mapsize));

	MDB_CHECK(mdb_env_open(env, path, 0, 0664));
	MDB_CHECK(mdb_txn_begin(env, NULL, 0, &txn));

	MDB_CHECK(mdb_dbi_open(txn, "db1", MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, &dbi1)); // Can use MDB_GET_MULTIPLE and MDB_NEXT_MULTIPLE
	MDB_CHECK(mdb_dbi_open(txn, "db2", MDB_INTEGERKEY | MDB_CREATE, &dbi2));

	/* #define DATANAME "/dictpopup_tmp_data.mdb" */
	/* #define LOCKNAME "/dictpopup_tmp_lock.mdb" */
	MDB_CHECK(mdb_env_create(&env_tmp));
	mdb_env_set_maxdbs(env_tmp, 3);
	MDB_CHECK(mdb_env_set_mapsize(env_tmp, mapsize));
	MDB_CHECK(mdb_env_open(env_tmp, "/tmp", 0, 0664));
	MDB_CHECK(mdb_txn_begin(env_tmp, NULL, 0, &txn_tmp));
	MDB_CHECK(mdb_dbi_open(txn_tmp, "db_tmp", MDB_CREATE, &dbi_tmp));
}

void
close_db()
{
	MDB_CHECK(mdb_txn_commit(txn));
	mdb_dbi_close(env, dbi1);
	mdb_dbi_close(env, dbi2);
	mdb_dbi_close(env, dbi_tmp);

	mdb_env_close(env);
	mdb_env_close(env_tmp);

	printf("Number of distinct entries: %i\n", entry_number);
}

void add_to_db(char* key_str, unsigned int keylen, char* value_str, unsigned int vallen)
{
	entry_number++;

	MDB_val key_s = { .mv_data = key_str, .mv_size = keylen };
	MDB_val value_s = { .mv_data = value_str, .mv_size = vallen };
	MDB_val id_s = { .mv_data = &entry_number, .mv_size = sizeof(entry_number) };

	// check if definition is already present. If not add it with new id
	rc = mdb_put(txn_tmp, dbi_tmp, &value_s, &id_s, MDB_NOOVERWRITE);
	if (rc == MDB_KEYEXIST)
		entry_number--;
	else
		MDB_CHECK(mdb_put(txn, dbi2, &id_s, &value_s, MDB_NOOVERWRITE | MDB_APPEND));

	// Add key with corresponding id
	mdb_put(txn, dbi1, &key_s, &id_s, MDB_NODUPDATA);
}
