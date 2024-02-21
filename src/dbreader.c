#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "util.h"
#include "settings.h" // for dbpath

#include "lmdb.h"

#define MDB_CHECK(call) \
	if ((rc = call) != MDB_SUCCESS) { \
		printf("Database error: %s\n", mdb_strerror(rc)); \
		exit(EXIT_FAILURE); \
	}

static int rc;
static MDB_env *env;
static MDB_dbi dbi1, dbi2;
static MDB_txn *txn;

void
open_database(void)
{
	MDB_CHECK(mdb_env_create(&env));
	mdb_env_set_maxdbs(env, 2);

	MDB_CHECK(mdb_env_open(env, cfg.db_path, MDB_RDONLY | MDB_NOLOCK | MDB_NORDAHEAD, 0664));
	MDB_CHECK(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));

	MDB_CHECK(mdb_dbi_open(txn, "db1", MDB_DUPSORT | MDB_DUPFIXED, &dbi1));
	MDB_CHECK(mdb_dbi_open(txn, "db2", MDB_INTEGERKEY, &dbi2));
}

void
close_database(void)
{
	mdb_txn_abort(txn);
	mdb_dbi_close(env, dbi1);
	mdb_dbi_close(env, dbi2);
	mdb_env_close(env);
}

static void
add_de_from_db_lookup(dictionary *dict, u8* data)
{
	u8* ptr_buffer[4];
	size_t len = 0;
	for (i32 i = 0; i < countof(ptr_buffer); i++)
	{
		len = strlen((char*)data);
		ptr_buffer[i] = data;
		data += len + 1;
	}

	dictionary_copy_add(dict, (dictentry) {
	    .dictname = (char*)ptr_buffer[0], 
	    .kanji = (char*)ptr_buffer[1], 
	    .reading = (char*)ptr_buffer[2], 
	    .definition = (char*)ptr_buffer[3] 
	    });
}

/*
 * wrapper for retarded API
 * Returns: NULL-terminated, decompressed data
 */
/* static char* */
/* decompress_data(const char* in, int in_size) */
/* { */
/* 	int buffer_size = 0, dlen = 0; */
/* 	char* buffer = NULL; */

/* 	do{ */
/* 		if ((buffer_size += 1000) < 0) */
/* 		{ */
/* 			fprintf(stderr, "Integer overflow when decompressing data.\n"); */
/* 			abort(); */
/* 		} */

/* 		buffer = g_realloc(buffer, (gsize)buffer_size); */
/* 		dlen = unishox2_decompress(in, in_size, UNISHOX_API_OUT_AND_LEN(buffer, buffer_size), USX_PSET_FAVOR_SYM); */
/* 	} while (dlen > buffer_size - 1); */

/* 	buffer[dlen] = '\0'; */
/* 	return buffer; */
/* } */


void
add_word_to_dict(dictionary *dict, s8 word)
{
	/* printf("Looking up: %s\n", word); */
	assert(word.len >= 0);
	MDB_val key = (MDB_val) { .mv_data = word.s, .mv_size = (size_t)word.len };
	MDB_val value;

	MDB_cursor *cursor;
	MDB_CHECK(mdb_cursor_open(txn, dbi1, &cursor));
	if ((rc = mdb_cursor_get(cursor, &key, &value, MDB_SET)) == MDB_NOTFOUND)
		return;
	Stopif(rc != MDB_SUCCESS, exit(1), "Database error: %s\n", mdb_strerror(rc));
	MDB_CHECK(mdb_cursor_get(cursor, &key, &value, MDB_GET_MULTIPLE)); // Reads up to a page, i.e. max 1024 entries

	u32* ids = value.mv_data;
	u32* end = (u32*)((u8*)value.mv_data + value.mv_size);
	for (uint32_t* id = ids; id < end; id++)
	{
		key = (MDB_val){ .mv_data = id, .mv_size = sizeof(u32) };

		MDB_val data;
		MDB_CHECK(mdb_get(txn, dbi2, &key, &data));

		/* char* decompressed = decompress_data(data.mv_data, (int)data.mv_size); */
		add_de_from_db_lookup(dict, (u8*)data.mv_data );
		/* g_free(decompressed); */
	}
}
