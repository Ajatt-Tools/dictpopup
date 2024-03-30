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

static MDB_env *env;
static MDB_dbi dbi1, dbi2;
static MDB_txn *txn;

void
open_database(void)
{
    MDB_CHECK(mdb_env_create(&env));
    mdb_env_set_maxdbs(env, 2);

    if (!cfg.general.dbpth)
      fatal("Path to the database file is NULL! This is a bug.");
    MDB_CHECK(mdb_env_open(env, cfg.general.dbpth, MDB_RDONLY | MDB_NOLOCK | MDB_NORDAHEAD, 0664));
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

    txn = 0;
    dbi1 = 0;
    dbi2 = 0;
    env = 0;
}

static void
add_de_from_db_lookup(dictentry* dict[static 1], s8 data)
{
    s8 d = data;

    s8 data_split[4] = { 0 };
    for (int i = 0; i < countof(data_split) && d.len > 0; i++)
    {
	size len = 0;
	for (; len < d.len; len++)
	{
	    if (d.s[len] == '\0')
		break;
	}
	data_split[i] = news8(len);
	u8copy(data_split[i].s, d.s, data_split[i].len);

	d.s += data_split[i].len + 1;
	d.len -= data_split[i].len + 1;
    }

    dictionary_add(dict, (dictentry) {
	.dictname = data_split[0],
	.kanji = data_split[1],
	.reading = data_split[2],
	.definition = data_split[3]
    });
}

void
add_word_to_dict(dictentry* dict[static 1], s8 word)
{
    debug_msg("Looking up: %.*s", (int)word.len, word.s);

    MDB_val key = (MDB_val) { .mv_data = word.s, .mv_size = (size_t)word.len };
    MDB_val value;

    MDB_cursor *cursor;
    MDB_CHECK(mdb_cursor_open(txn, dbi1, &cursor));

    if ((rc = mdb_cursor_get(cursor, &key, &value, MDB_SET)) == MDB_NOTFOUND)
	return;
    MDB_CHECK(rc);

    MDB_CHECK(mdb_cursor_get(cursor, &key, &value, MDB_GET_MULTIPLE));     // Reads up to a page, i.e. max 1024 entries

    u32* ids = value.mv_data;
    u32* end = (u32*)((u8*)value.mv_data + value.mv_size);
    for (uint32_t* id = ids; id < end; id++)
    {
	key = (MDB_val){ .mv_data = id, .mv_size = sizeof(u32) };

	MDB_val data;
	MDB_CHECK(mdb_get(txn, dbi2, &key, &data));

	add_de_from_db_lookup(dict, (s8){ .s = data.mv_data, .len=data.mv_size });
    }
}
