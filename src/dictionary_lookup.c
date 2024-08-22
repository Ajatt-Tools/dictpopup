#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include "db.h"
#include "deinflector.h"
#include "dictionary_lookup.h"
#include "utils/messages.h"
#include "utils/utf8.h"
#include "utils/util.h"

#include "platformdep/file_operations.h"
#include "platformdep/file_paths.h"
#include "platformdep/windowtitle.h"
#include "utils/str.h"
#ifdef CLIPBOARD
    #include "platformdep/clipboard.h"
#endif

// This only applies for substring search
// 60 are 20 Japanese characters
#define MAX_LOOKUP_LEN 60 // in bytes

static void _nonnull_ append_deinflections(const database_t *db, s8 word, Dict dict[static 1]) {
    _drop_(s8_buf_free) s8Buf deinfs_b = deinflect(word);

    if (deinfs_b == NULL)
        return;

    for (size_t i = 0; i < buf_size(deinfs_b); i++)
        db_append_lookup(db, deinfs_b[i], dict, true);
}

static Dict _nonnull_ lookup_word(s8 word, database_t *db) {
    Dict dict = newDict();
    db_append_lookup(db, word, &dict, false);
    append_deinflections(db, word, &dict);
    return dict;
}

static Dict _nonnull_ lookup_first_matching_prefix(s8 *word, database_t *db) {
    Dict dict = newDict();

    int firstChrLen = utf8_chr_len(word->s);
    while (isEmpty(dict) && word->len > firstChrLen) {
        stripUtf8Char(word);
        if (word->len < MAX_LOOKUP_LEN) { // Don't waste time looking up huge strings
            dict = lookup_word(*word, db);
        }
    }

    return dict;
}

static Dict _nonnull_ lookup_hiragana_conversion(s8 word, database_t *db) {
    _drop_(frees8) s8 hira = kanji2hira(word);
    return lookup_word(hira, db);
}

static DictLookup _nonnull_ lookup(s8 word, DictpopupConfig cfg) {
    _drop_(db_close) database_t *db = db_open(true);

    Dict dict = lookup_word(word, db);
    if (isEmpty(dict) && cfg.fallback_to_mecab_conversion) {
        dict = lookup_hiragana_conversion(word, db);
    }
    if (isEmpty(dict) && cfg.lookup_longest_matching_prefix) {
        dict = lookup_first_matching_prefix(&word, db);
    }

    return (DictLookup){.dict = dict, .lookup = word};
}

DictLookup _nonnull_ dictionary_lookup(s8 word, DictpopupConfig cfg) {
    if (cfg.nuke_whitespace_of_lookup)
        nuke_whitespace(&word);

    DictLookup dict_lookup = lookup(word, cfg);

    if (cfg.dict_sort_order)
        dict_sort(dict_lookup.dict, cfg.dict_sort_order);

    return dict_lookup;
}

// static void copy_default_database_to(char *path) {
//     const char *default_db_loc = NULL;
//     for (size_t i = 0; i < arrlen(DEFAULT_DATABASE_LOCATIONS); i++) {
//         if (check_file_exists(DEFAULT_DATABASE_LOCATIONS[i])) {
//             default_db_loc = DEFAULT_DATABASE_LOCATIONS[i];
//             break;
//         }
//     }
//     die_on(default_db_loc == NULL,
//            "Could not access the default database either. You need to create your own with"
//            "dictpopup-create or download data.mdb from the repository.");
//
//     createdir(path);
//     _drop_(frees8) s8 dbpath = buildpath(fromcstr_(path), S("data.mdb"));
//     file_copy_sync(default_db_loc, (char *)dbpath.s);
// }

// void dictpopup_init(DictpopupConfig cfg) {
//     setlocale(LC_ALL, "");
//
//     if (!db_check_exists(fromcstr_((char*)cfg.database_dir))) {
//         msg("No database found. We recommend creating your own database with dictpopup-create,
//         but "
//             "copying default dictionary for now..");
//         copy_default_database_to((char*)cfg.database_dir);
//     }
//
//     focused_window_title = get_windowname();
// }
