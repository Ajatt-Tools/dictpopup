#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lmdb.h>

#include "unishox2.h"

#include "dictionary.h"
#include "readsettings.h"

#define MDB_CHECK(call) \
	if ((rc = call) != MDB_SUCCESS && rc != MDB_NOTFOUND) { \
		printf("Database error: %s\n", mdb_strerror(rc)); \
		exit(EXIT_FAILURE); \
	}

int rc;
MDB_env *env;
MDB_dbi dbi1, dbi2;
MDB_txn *txn;

void
open_database()
{
	MDB_CHECK(mdb_env_create(&env));
	mdb_env_set_maxdbs(env, 2);

	MDB_CHECK(mdb_env_open(env, cfg.db_path, MDB_RDONLY | MDB_NOLOCK | MDB_NORDAHEAD, 0664));
	MDB_CHECK(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));

	MDB_CHECK(mdb_dbi_open(txn, "db1", MDB_DUPSORT | MDB_DUPFIXED, &dbi1));
	MDB_CHECK(mdb_dbi_open(txn, "db2", MDB_INTEGERKEY, &dbi2));
}

void
close_database()
{
	mdb_txn_abort(txn);
	mdb_dbi_close(env, dbi1);
	mdb_dbi_close(env, dbi2);
	mdb_env_close(env);
}

void
print_entry(char *entry, size_t entry_len)
{
	for (char *c = entry; c != entry + entry_len; c++)
	{
		if (*c)
			printf("%c", *c);
		else
			printf("\n");
	}
	printf("\n\n");
}

char*
add_de_from_db_lookup(dictionary *dict, char *db_lookup)
{
	char *ptr_buffer[4];

	int len = 0;
	for (int i = 0; i < 4; i++)
	{
		len = strlen(db_lookup);
		ptr_buffer[i] = db_lookup;
		db_lookup += len + 1;
	}

	dictionary_copy_add(dict, (const dictentry) { .dictname = ptr_buffer[0], .kanji = ptr_buffer[1], .reading = ptr_buffer[2], .definition = ptr_buffer[3] });
	return db_lookup;
}

void
add_word_to_dict(dictionary *dict, char *word)
{
	printf("Looking up: %s\n", word);

	MDB_val key = (MDB_val) { .mv_data = word, .mv_size = strlen(word) };
	MDB_val ids;

	MDB_cursor *cursor;
	MDB_CHECK(mdb_cursor_open(txn, dbi1, &cursor));
	MDB_CHECK(mdb_cursor_get(cursor, &key, &ids, MDB_SET));

	MDB_CHECK(mdb_cursor_get(cursor, &key, &ids, MDB_GET_MULTIPLE)); // Reads up to a page, i.e. max 1024 entries
	for (int i = 0; i < ids.mv_size ; i += 4)
	{
		MDB_val id = { .mv_data = (unsigned char*)ids.mv_data + i, .mv_size = 4 };

		MDB_val data;
		MDB_CHECK(mdb_get(txn, dbi2, &id, &data));
		char decompressed[5000]; // FIXME: This is obviously bad, have to lookup the proper way of decompressing
		size_t len = unishox2_decompress_simple(data.mv_data, data.mv_size, decompressed);
		decompressed[len] = '\0';

		add_de_from_db_lookup(dict, decompressed);
	}
}
