#include <lmdb.h>
#include <string.h>

#include "db.h"
#include "platformdep/file_operations.h"
#include "utils/messages.h"

#define DICTNAMES_KEY_STR "%STORED_DICTIONARIES%"
#define DB_DEFAULT_MAPSIZE (2ULL * 1024 * 1024 * 1024) // 2GB
#define DB_MAX_DBS 4
#define DB_WORDS_TO_ID "db1"
#define DB_ID_TO_DEFINITIONS "db2"
#define DB_FREQUENCIES "db3"
#define DB_METADATA "db4"

DEFINE_DROP_FUNC(MDB_cursor *, mdb_cursor_close)

struct database_s {
    MDB_env *env;
    MDB_dbi db_words_to_id;
    MDB_dbi db_id_to_definition;
    MDB_dbi db_frequencies;
    MDB_dbi db_metadata;
    MDB_txn *txn;
    stringbuilder_s lastdatastr;
    stringbuilder_s datastr;
    u32 last_id;
    bool readonly;
};

#define DB_TRY(call)                                                                               \
    do {                                                                                           \
        int _rc = (call);                                                                          \
        die_on(_rc != MDB_SUCCESS, "Database error: %s", mdb_strerror(_rc));                       \
    } while (0)

s8 db_get_dbpath(void) {
    return buildpath(fromcstr_((char *)get_user_data_dir()), S("dictpopup"));
}

database_t *db_open(s8 dbdir, bool readonly) {
    dbg("Opening database in directory: '%s'", (char *)dbdir.s);

    database_t *db = new (database_t, 1);
    db->readonly = readonly;

    DB_TRY(mdb_env_create(&db->env));
    mdb_env_set_maxdbs(db->env, DB_MAX_DBS);

    if (readonly) {
        die_on(!db_check_exists(dbdir),
               "There is no database in '%s'. You must create one first with dictpopup-create.",
               (char *)dbdir.s);

        DB_TRY(
            mdb_env_open(db->env, (char *)dbdir.s, MDB_RDONLY | MDB_NOLOCK | MDB_NORDAHEAD, 0664));
        DB_TRY(mdb_txn_begin(db->env, NULL, MDB_RDONLY, &db->txn));

        DB_TRY(
            mdb_dbi_open(db->txn, DB_WORDS_TO_ID, MDB_DUPSORT | MDB_DUPFIXED, &db->db_words_to_id));
        DB_TRY(
            mdb_dbi_open(db->txn, DB_ID_TO_DEFINITIONS, MDB_INTEGERKEY, &db->db_id_to_definition));
        DB_TRY(mdb_dbi_open(db->txn, DB_FREQUENCIES, 0, &db->db_frequencies));
        DB_TRY(mdb_dbi_open(db->txn, DB_METADATA, 0, &db->db_metadata));
    } else {
        unsigned int mapsize = 2097152000; // 2Gb
        DB_TRY(mdb_env_set_mapsize(db->env, mapsize));

        DB_TRY(mdb_env_open(db->env, (char *)dbdir.s, 0, 0664));
        DB_TRY(mdb_txn_begin(db->env, NULL, 0, &db->txn));

        DB_TRY(mdb_dbi_open(db->txn, DB_WORDS_TO_ID, MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE,
                            &db->db_words_to_id));
        DB_TRY(mdb_dbi_open(db->txn, DB_ID_TO_DEFINITIONS, MDB_INTEGERKEY | MDB_CREATE,
                            &db->db_id_to_definition));
        DB_TRY(mdb_dbi_open(db->txn, DB_FREQUENCIES, MDB_CREATE, &db->db_frequencies));
        DB_TRY(mdb_dbi_open(db->txn, DB_METADATA, MDB_CREATE, &db->db_metadata));

        db->datastr = sb_init(200);
        db->lastdatastr = sb_init(200);
    }

    return db;
}

void db_close(database_t *db) {
    if (db->readonly) {
        mdb_txn_abort(db->txn);
    } else {
        DB_TRY(mdb_txn_commit(db->txn));

        sb_free(&db->datastr);
        sb_free(&db->lastdatastr);

        const char *dbpath = NULL;
        mdb_env_get_path(db->env, &dbpath);
        _drop_(frees8) s8 lockfile = buildpath(fromcstr_((char *)dbpath), S("lock.mdb"));
        remove((char *)lockfile.s);
    }

    mdb_dbi_close(db->env, db->db_words_to_id);
    mdb_dbi_close(db->env, db->db_id_to_definition);
    mdb_dbi_close(db->env, db->db_frequencies);
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

static void put_de_if_new(database_t *db, dictentry de) {
    MDB_val id_mdb = {.mv_data = &db->last_id, .mv_size = sizeof(db->last_id)};
    serialize_dictentry(&db->datastr, de);
    if (!s8equals(sb_gets8(db->datastr), sb_gets8(db->lastdatastr))) {
        s8 datastr = sb_gets8(db->datastr);
        db->last_id++; // Note: The above id struct updates too
        MDB_val val_mdb = {.mv_data = datastr.s, .mv_size = datastr.len};
        DB_TRY(mdb_put(db->txn, db->db_id_to_definition, &id_mdb, &val_mdb,
                       MDB_NOOVERWRITE | MDB_APPEND));

        stringbuilder_s tmp = db->lastdatastr;
        db->lastdatastr = db->datastr;
        db->datastr = tmp;
    }
}

static void add_key_with_id(database_t *db, s8 key, u32 id) {
    MDB_val key_mdb = {.mv_data = key.s, .mv_size = key.len};
    MDB_val id_mdb = {.mv_data = &id, .mv_size = sizeof(id)};
    mdb_put(db->txn, db->db_words_to_id, &key_mdb, &id_mdb, MDB_NODUPDATA);
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

    put_de_if_new(db, de);
    add_key_with_id(db, de.kanji, db->last_id);
    add_key_with_id(db, de.reading, db->last_id);
}

static u32 *get_ids(const database_t *db, s8 word, size_t *num) {
    MDB_val key_mdb = (MDB_val){.mv_data = word.s, .mv_size = (size_t)word.len};
    MDB_val val_mdb = {0};

    _drop_(mdb_cursor_close) MDB_cursor *cursor;
    DB_TRY(mdb_cursor_open(db->txn, db->db_words_to_id, &cursor));

    int rc = mdb_cursor_get(cursor, &key_mdb, &val_mdb, MDB_SET);
    if (rc == MDB_NOTFOUND)
        return NULL;
    DB_TRY(rc);
    // This reads up to a page, i.e. max 1024 entries, which should be enough
    DB_TRY(mdb_cursor_get(cursor, &key_mdb, &val_mdb, MDB_GET_MULTIPLE));

    *num = val_mdb.mv_size / sizeof(u32);
    assume(*num * sizeof(u32) == val_mdb.mv_size); // divides cleanly
    u32 *ret = new (u32, *num);
    memcpy(ret, val_mdb.mv_data, val_mdb.mv_size); // ensures proper alignment
    return ret;
}

void db_put_freq(database_t *db, freqentry fe) {
    die_on(db->readonly, "Cannot put frequency into db in readonly mode.");

    s8 key = concat(fe.word, S("\0"), fe.reading);
    MDB_val key_mdb = {.mv_data = key.s, .mv_size = key.len};
    MDB_val val_mdb = {.mv_data = &fe.frequency, .mv_size = sizeof(fe.frequency)};

    DB_TRY(mdb_put(db->txn, db->db_frequencies, &key_mdb, &val_mdb, MDB_NODUPDATA));
}

static u32 db_get_freq(const database_t *db, s8 word, s8 reading) {
    _drop_(frees8) s8 key = concat(word, S("\0"), reading);
    MDB_val key_m = (MDB_val){.mv_data = key.s, .mv_size = (size_t)key.len};

    MDB_val val_m = {0};
    int rc = mdb_get(db->txn, db->db_frequencies, &key_m, &val_m);
    if (rc == MDB_NOTFOUND)
        return 0;
    DB_TRY(rc);

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
    DB_TRY(mdb_get(db->txn, db->db_id_to_definition, &key, &data));
    return (s8){.s = data.mv_data, .len = data.mv_size};
}

// TODO: Refactor this
void db_append_lookup(const database_t *db, s8 headword, Dict dict[static 1],
                      bool is_deinflection) {
    dbg("Looking up: %.*s", (int)headword.len, (char *)headword.s);

    size_t n_ids = 0;
    _drop_(free) u32 *ids = get_ids(db, headword, &n_ids);
    if (ids) {
        for (size_t i = 0; i < n_ids; i++) {
            s8 de_data = getdata(db, ids[i]);
            dictentry de = data_to_dictent(db, de_data);
            de.is_deinflection = is_deinflection;
            dictionary_add(dict, de);
        }
    }
}

// TODO: Check for duplicates
void _nonnull_ db_put_dictname(database_t *db, s8 dictname) {
    MDB_val key = (MDB_val){.mv_data = DICTNAMES_KEY_STR, .mv_size = lengthof(DICTNAMES_KEY_STR)};

    MDB_val data;
    int rc = mdb_get(db->txn, db->db_metadata, &key, &data);
    if (rc == MDB_NOTFOUND) {
        // This is the first entry
        data.mv_data = dictname.s;
        data.mv_size = dictname.len;
    } else if (rc == 0) {
        // There are already other entries
        s8 existing = {.s = data.mv_data, .len = data.mv_size};
        s8 new_value = concat(existing, S("\0"), dictname);

        data.mv_size = new_value.len;
        data.mv_data = new_value.s;
    } else {
        DB_TRY(rc);
    }

    DB_TRY(mdb_put(db->txn, db->db_metadata, &key, &data, 0));

    if (data.mv_data != dictname.s) {
        free(data.mv_data);
    }
}

s8Buf db_get_dictnames(database_t *db) {
    MDB_val key = (MDB_val){.mv_data = DICTNAMES_KEY_STR, .mv_size = lengthof(DICTNAMES_KEY_STR)};

    MDB_val data;
    int rc = mdb_get(db->txn, db->db_metadata, &key, &data);

    if (rc == MDB_NOTFOUND) {
        return NULL;
    }
    DB_TRY(rc);

    s8 all_names = {.s = data.mv_data, .len = data.mv_size};
    s8 *names = NULL;

    size_t start = 0;
    for (isize i = 0; i <= all_names.len; i++) {
        if (i == all_names.len || all_names.s[i] == '\0') {
            s8 name = s8dup((s8){.s = &all_names.s[start], .len = i - start});
            buf_push(names, name);
            start = i + 1;
        }
    }

    return names;
}

bool db_check_exists(s8 dbpath) {
    _drop_(frees8) s8 dbfile = buildpath(dbpath, S("data.mdb"));
    return check_file_exists((char *)dbfile.s);
}

void db_remove(s8 dbpath) {
    _drop_(frees8) s8 dbfile = buildpath(dbpath, S("data.mdb"));
    rem((char *)dbfile.s);
}