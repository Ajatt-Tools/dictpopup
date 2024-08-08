#include <lmdb.h>
#include <string.h>

#include "db.h"
#include "platformdep/file_operations.h"
#include "utils/messages.h"

DEFINE_DROP_FUNC(MDB_cursor *, mdb_cursor_close)

struct database_s {
    MDB_env *env;
    MDB_dbi dbi1;
    MDB_dbi dbi2;
    MDB_dbi dbi3;
    MDB_txn *txn;
    stringbuilder_s lastdatastr;
    stringbuilder_s datastr;
    u32 last_id;
    bool readonly;
};

static int rc = 0;
#define C(call) die_on((rc = (call)) != MDB_SUCCESS, "Database error: %s", mdb_strerror(rc));

database_t *db_open(const char *dbpath, bool readonly) {
    dbg("Opening database in directory: '%s'", dbpath);

    database_t *db = new (database_t, 1);
    db->readonly = readonly;

    C(mdb_env_create(&db->env));
    mdb_env_set_maxdbs(db->env, 3);

    if (readonly) {
        die_on(!db_check_exists(fromcstr_((char*)dbpath)),
               "There is no database in '%s'. You must create one first with dictpopup-create.",
               dbpath);

        C(mdb_env_open(db->env, dbpath, MDB_RDONLY | MDB_NOLOCK | MDB_NORDAHEAD, 0664));
        C(mdb_txn_begin(db->env, NULL, MDB_RDONLY, &db->txn));

        C(mdb_dbi_open(db->txn, "db1", MDB_DUPSORT | MDB_DUPFIXED, &db->dbi1));
        C(mdb_dbi_open(db->txn, "db2", MDB_INTEGERKEY, &db->dbi2));
        C(mdb_dbi_open(db->txn, "db3", 0, &db->dbi3));
    } else {
        unsigned int mapsize = 2097152000; // 2Gb
        C(mdb_env_set_mapsize(db->env, mapsize));

        C(mdb_env_open(db->env, dbpath, 0, 0664));
        C(mdb_txn_begin(db->env, NULL, 0, &db->txn));

        // word -> id
        C(mdb_dbi_open(db->txn, "db1", MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, &db->dbi1));
        // id -> dictdef
        C(mdb_dbi_open(db->txn, "db2", MDB_INTEGERKEY | MDB_CREATE, &db->dbi2));
        // word+reading -> frequency
        C(mdb_dbi_open(db->txn, "db3", MDB_CREATE, &db->dbi3));

        db->datastr = sb_init(200);
        db->lastdatastr = sb_init(200);
    }

    return db;
}

void db_close(database_t *db) {
    if (db->readonly) {
        mdb_txn_abort(db->txn);
    } else {
        C(mdb_txn_commit(db->txn));

        sb_free(&db->datastr);
        sb_free(&db->lastdatastr);

        const char *dbpath = NULL;
        mdb_env_get_path(db->env, &dbpath);
        _drop_(frees8) s8 lockfile = buildpath(fromcstr_((char *)dbpath), S("lock.mdb"));
        remove((char *)lockfile.s);
    }

    mdb_dbi_close(db->env, db->dbi1);
    mdb_dbi_close(db->env, db->dbi2);
    mdb_dbi_close(db->env, db->dbi3);
    mdb_env_close(db->env);
    free(db);
}

static void serialize_dictentry(stringbuilder_s sb[static 1], dictentry de) {
    s8 sep = S("\0");
    sb_set(sb, de.dictname);
    sb_append(sb, sep);
    sb_append(sb, de.kanji);
    sb_append(sb, sep);
    sb_append(sb, de.reading);
    sb_append(sb, sep);
    sb_append(sb, de.definition);
}

void db_put_dictent(database_t *db, dictentry de) {
    die_on(db->readonly, "Cannot put dictentry into db in readonly mode.");
    if (!de.definition.len) {
        dbg("Entry with kanji: %s and reading: %s has no definition. Skipping..",
            (char *)de.kanji.s, (char *)de.reading.s);
        return;
    }
    if (!de.kanji.len && !de.reading.len) {
        dbg("Entry with definition: %s has no kanji and reading. Skipping..",
            (char *)de.definition.s);
        return;
    }

    MDB_val kanji_mdb = {.mv_data = de.kanji.s, .mv_size = de.kanji.len};
    MDB_val reading_mdb = {.mv_data = de.reading.s, .mv_size = de.reading.len};
    MDB_val id_mdb = {.mv_data = &db->last_id, .mv_size = sizeof(db->last_id)};

    serialize_dictentry(&db->datastr, de);

    if (!s8equals(sb_gets8(db->datastr), sb_gets8(db->lastdatastr))) {
        s8 datastr = sb_gets8(db->datastr);
        db->last_id++; // Note: The above id struct updates too
        MDB_val val_mdb = {.mv_data = datastr.s, .mv_size = datastr.len};
        C(mdb_put(db->txn, db->dbi2, &id_mdb, &val_mdb, MDB_NOOVERWRITE | MDB_APPEND));

        stringbuilder_s tmp = db->lastdatastr;
        db->lastdatastr = db->datastr;
        db->datastr = tmp;
    }

    // Add key with corresponding id
    mdb_put(db->txn, db->dbi1, &kanji_mdb, &id_mdb, MDB_NODUPDATA);
    mdb_put(db->txn, db->dbi1, &reading_mdb, &id_mdb, MDB_NODUPDATA);
}

static u32 *getids(const database_t *db, s8 word, size_t *num) {
    MDB_val key_mdb = (MDB_val){.mv_data = word.s, .mv_size = (size_t)word.len};
    MDB_val val_mdb = {0};

    _drop_(mdb_cursor_close) MDB_cursor *cursor;
    C(mdb_cursor_open(db->txn, db->dbi1, &cursor));

    rc = mdb_cursor_get(cursor, &key_mdb, &val_mdb, MDB_SET);
    if (rc == MDB_NOTFOUND)
        return NULL;
    C(rc);
    // This reads up to a page, i.e. max 1024 entries, which should be enough
    C(mdb_cursor_get(cursor, &key_mdb, &val_mdb, MDB_GET_MULTIPLE));

    *num = val_mdb.mv_size / sizeof(u32);
    assume(*num * sizeof(u32) == val_mdb.mv_size); // divides cleanly
    u32 *ret = new (u32, *num);
    memcpy(ret, val_mdb.mv_data, val_mdb.mv_size); // ensures proper alignment
    return ret;
}

void db_put_freq(const database_t *db, freqentry fe) {
    die_on(db->readonly, "Cannot put frequency into db in readonly mode.");

    s8 key = concat(fe.word, S("\0"), fe.reading);
    MDB_val key_mdb = {.mv_data = key.s, .mv_size = key.len};
    MDB_val val_mdb = {.mv_data = &fe.frequency, .mv_size = sizeof(fe.frequency)};

    C(mdb_put(db->txn, db->dbi3, &key_mdb, &val_mdb, MDB_NODUPDATA));
}

static u32 db_get_freq(const database_t *db, s8 word, s8 reading) {
    _drop_(frees8) s8 key = concat(word, S("\0"), reading);
    MDB_val key_m = (MDB_val){.mv_data = key.s, .mv_size = (size_t)key.len};

    MDB_val val_m = {0};
    rc = mdb_get(db->txn, db->dbi3, &key_m, &val_m);
    if (rc == MDB_NOTFOUND)
        return 0;
    C(rc);

    u32 freq;
    assume(sizeof(freq) == val_m.mv_size);
    memcpy(&freq, val_m.mv_data, val_m.mv_size); // ensures proper alignment
    return freq;
}

/*
 * Returns: dictentry with newly allocated strings parsed from @data
 */
static dictentry data_to_dictent(const database_t *db, s8 data) {
    assert(data.s);

    s8 data_split[4] = {0};
    for (size_t i = 0; i < arrlen(data_split); i++) {
        assume(data.len >= 0);

        isize len = 0;
        while (len < data.len && data.s[len] != '\0')
            len++;
        data_split[i] = news8(len);
        memcpy(data_split[i].s, data.s, data_split[i].len);

        data.s += data_split[i].len + 1;
        data.len -= data_split[i].len + 1;
    }

    dictentry ret = {0};
    ret.dictname = data_split[0];
    ret.kanji = data_split[1];
    ret.reading = data_split[2];
    ret.definition = data_split[3];
    ret.frequency = db_get_freq(db, ret.kanji, ret.reading);
    return ret;
}

/*
 * Returns: Data associated to id from second database.
 * Data is valid until closure of db and should not be freed.
 * WARNING: returned data is not null-terminated!
 */
static s8 getdata(const database_t *db, u32 id) {
    MDB_val key = (MDB_val){.mv_data = &id, .mv_size = sizeof(id)};
    MDB_val data = {0};
    C(mdb_get(db->txn, db->dbi2, &key, &data));
    return (s8){.s = data.mv_data, .len = data.mv_size};
}

// TODO: Refactor this
void db_append_lookup(const database_t *db, s8 headword, Dict dict[static 1],
                      bool is_deinflection) {
    dbg("Looking up: %.*s", (int)headword.len, (char *)headword.s);

    size_t n_ids = 0;
    _drop_(free) u32 *ids = getids(db, headword, &n_ids);
    if (ids) {
        for (size_t i = 0; i < n_ids; i++) {
            s8 de_data = getdata(db, ids[i]);
            dictentry de = data_to_dictent(db, de_data);
            de.is_deinflection = is_deinflection;
            dictionary_add(dict, de);
        }
    }
}

bool db_check_exists(s8 dbpath) {
    _drop_(frees8) s8 dbfile = buildpath(dbpath, S("data.mdb"));
    return check_file_exists((char *)dbfile.s);
}

void db_remove(s8 dbpath) {
    _drop_(frees8) s8 dbfile = buildpath(dbpath, S("data.mdb"));
    rem((char *)dbfile.s);
}
