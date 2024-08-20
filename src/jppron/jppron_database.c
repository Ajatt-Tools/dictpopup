#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <lmdb.h>
#include <string.h>

#include "jppron/ajt_audio_index_parser.h"
#include "utils/messages.h"
#include "utils/str.h"
#include "utils/util.h"

#include <jppron/jppron_database.h>

DEFINE_DROP_FUNC(MDB_cursor *, mdb_cursor_close)

#define MDB_CHECK(call)                                                                            \
    do {                                                                                           \
        int _rc = (call);                                                                          \
        die_on(_rc != MDB_SUCCESS, "Database error: %s", mdb_strerror(_rc));                       \
    } while (0)

struct _PronDatabase {
    stringbuilder_s lastval;
    MDB_env *env;
    MDB_dbi db_headword_to_filepath;
    MDB_dbi db_filepath_to_fileinfo;
    MDB_txn *txn;
    bool readonly;
};

PronDatabase *jppron_open_db(char *dbpath, bool readonly) {
    dbg("Opening database in directory: '%s'", dbpath);

    PronDatabase *db = new (PronDatabase, 1);
    db->readonly = readonly;

    MDB_CHECK(mdb_env_create(&db->env));
    mdb_env_set_maxdbs(db->env, 2);

    if (db->readonly) {
        die_on(!jdb_check_exists(fromcstr_(dbpath)), "There is no jppron database in '%s'.",
               dbpath);

        MDB_CHECK(mdb_env_open(db->env, dbpath, MDB_RDONLY | MDB_NOLOCK | MDB_NORDAHEAD, 0664));

    } else {
        unsigned int mapsize = 2147483648; // 2 Gb max
        MDB_CHECK(mdb_env_set_mapsize(db->env, mapsize));

        MDB_CHECK(mdb_env_open(db->env, dbpath, 0, 0664));
        MDB_CHECK(mdb_txn_begin(db->env, NULL, 0, &db->txn));
        MDB_CHECK(
            mdb_dbi_open(db->txn, "dbi1", MDB_DUPSORT | MDB_CREATE, &db->db_headword_to_filepath));
        MDB_CHECK(mdb_dbi_open(db->txn, "dbi2", MDB_CREATE, &db->db_filepath_to_fileinfo));

        db->lastval = sb_init(200);
    }

    return db;
}

void jppron_close_db(PronDatabase *db) {
    if (!db->readonly) {
        MDB_CHECK(mdb_txn_commit(db->txn));
        mdb_dbi_close(db->env, db->db_headword_to_filepath);
        mdb_dbi_close(db->env, db->db_filepath_to_fileinfo);

        const char *dbpath = NULL;
        mdb_env_get_path(db->env, &dbpath);
        _drop_(frees8) s8 lockfile = buildpath(fromcstr_((char *)dbpath), S("lock.mdb"));
        remove((char *)lockfile.s);
    }

    mdb_env_close(db->env);
    free(db);
}

void jdb_add_headword_with_file(PronDatabase *db, s8 headword, s8 filepath) {
    MDB_val mdb_key = {.mv_data = headword.s, .mv_size = (size_t)headword.len};
    MDB_val mdb_val = {.mv_data = filepath.s, .mv_size = (size_t)filepath.len};

    MDB_CHECK(mdb_put(db->txn, db->db_headword_to_filepath, &mdb_key, &mdb_val, 0));
}

static s8 serialize_fileinfo(FileInfo fi) {
    return concat(fi.origin, S("\0"), fi.hira_reading, S("\0"), fi.pitch_number, S("\0"),
                  fi.pitch_pattern);
}

static FileInfo deserialize_to_fileinfo(s8 data) {
    s8 data_split[4] = {0};
    for (long unsigned int i = 0; i < arrlen(data_split) && data.len > 0; i++) {
        assert(data.len > 0);

        isize len = 0;
        while (len < data.len && data.s[len] != '\0')
            len++;

        data_split[i] = news8(len);
        memcpy(data_split[i].s, data.s, data_split[i].len);

        data.s += data_split[i].len + 1;
        data.len -= data_split[i].len + 1;
    }

    return (FileInfo){.origin = data_split[0],
                      .hira_reading = data_split[1],
                      .pitch_number = data_split[2],
                      .pitch_pattern = data_split[3]};
}

void jdb_add_file_with_fileinfo(PronDatabase *db, s8 filepath, FileInfo fi) {
    _drop_(frees8) s8 data = serialize_fileinfo(fi);

    MDB_val mdb_key = {.mv_data = filepath.s, .mv_size = (size_t)filepath.len};
    MDB_val mdb_val = {.mv_data = data.s, .mv_size = (size_t)data.len};
    MDB_CHECK(mdb_put(db->txn, db->db_filepath_to_fileinfo, &mdb_key, &mdb_val, MDB_NOOVERWRITE));
}

static s8 get_fileinfo_data(PronDatabase *db, s8 fullpath) {
    int rc;
    MDB_CHECK(mdb_txn_begin(db->env, NULL, MDB_RDONLY, &db->txn));
    MDB_CHECK(mdb_dbi_open(db->txn, "dbi2", 0, &db->db_filepath_to_fileinfo));

    MDB_val key_m = (MDB_val){.mv_data = fullpath.s, .mv_size = (size_t)fullpath.len};
    MDB_val val_m = {0};

    if ((rc = mdb_get(db->txn, db->db_filepath_to_fileinfo, &key_m, &val_m)) == MDB_NOTFOUND)
        return (s8){0};
    MDB_CHECK(rc);

    s8 ret = s8dup((s8){.s = val_m.mv_data, .len = val_m.mv_size});

    mdb_txn_abort(db->txn);
    return ret;
}

FileInfo jdb_get_fileinfo(PronDatabase *db, s8 fullpath) {
    _drop_(frees8) s8 data = get_fileinfo_data(db, fullpath);
    return deserialize_to_fileinfo(data);
}

s8Buf jdb_get_files(PronDatabase *db, s8 word) {
    int rc;
    MDB_CHECK(mdb_txn_begin(db->env, NULL, MDB_RDONLY, &db->txn));
    MDB_CHECK(mdb_dbi_open(db->txn, "dbi1", MDB_DUPSORT, &db->db_headword_to_filepath));

    MDB_val key_m = (MDB_val){.mv_data = word.s, .mv_size = (size_t)word.len};
    MDB_val val_m = {0};

    _drop_(mdb_cursor_close) MDB_cursor *cursor = 0;
    MDB_CHECK(mdb_cursor_open(db->txn, db->db_headword_to_filepath, &cursor));

    s8 *ret = 0;
    bool first = true;
    while ((rc = mdb_cursor_get(cursor, &key_m, &val_m, first ? MDB_SET_KEY : MDB_NEXT_DUP)) == 0) {
        s8 val = s8dup((s8){.s = val_m.mv_data, .len = val_m.mv_size});
        buf_push(ret, val);

        first = false;
    }
    if (rc != MDB_NOTFOUND)
        MDB_CHECK(rc);

    mdb_txn_abort(db->txn);
    return ret;
}

i32 jdb_check_exists(s8 dbpath) {
    _drop_(frees8) s8 dbfile = buildpath(dbpath, S("data.mdb"));
    return (access((char *)dbfile.s, R_OK) == 0);
}

void jdb_remove(s8 dbpath) {
    _drop_(frees8) s8 dbfile = buildpath(dbpath, S("data.mdb"));
    remove((char *)dbfile.s);
}