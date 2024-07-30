#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "ankiconnectc.h"
#include "db.h"
#include "deinflector.h"
#include "dictpopup.h"
#include "settings.h"
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

s8 focused_window_title = {0};

#define POSSIBLE_ENTRIES_S_NMEMB 9
typedef struct possible_entries_s {
    s8 copiedsent;
    s8 boldsent;
    s8 dictkanji;
    s8 dictreading;
    s8 dictdefinition;
    s8 furigana;
    s8 windowname;
    s8 dictname;
} possible_entries_s;

static void _nonnull_ appendDeinflections(const database_t *db, s8 word,
                                          dictentry *dict[static 1]) {
    _drop_(frees8buffer) s8 *deinfs_b = deinflect(word);

    for (size_t i = 0; i < buf_size(deinfs_b); i++)
        db_append_lookup(db, deinfs_b[i], dict, true);
}

static Dict _nonnull_ lookup_word(s8 word, database_t *db) {
    Dict dict = newDict();
    db_append_lookup(db, word, &dict, false);
    appendDeinflections(db, word, &dict);
    return dict;
}

static Dict _nonnull_ lookup_first_matching_prefix(s8 *word, database_t *db) {
    dictentry *dict = newDict();

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

static Dict _nonnull_ lookup(s8 *word, Config config) {
    _drop_(db_close) database_t *db = db_open(config.general.dbDir, true);

    Dict dict = lookup_word(*word, db);
    if (isEmpty(dict) && config.general.mecab) {
        dict = lookup_hiragana_conversion(*word, db);
    }
    if (isEmpty(dict) && config.general.substringSearch) {
        dict = lookup_first_matching_prefix(word, db);
    }
    return dict;
}

static int indexof(char const *str, char *arr[]) {
    if (str && arr) {
        for (int i = 0; arr[i]; i++) {
            if (strcmp(str, arr[i]) == 0)
                return i;
        }
    }
    return INT_MAX;
}

/*
 * -1 means @a comes first
 */
static int _nonnull_ dictentry_comparer(const dictentry *a, const dictentry *b) {
    int inda, indb;

    // TODO: Make this less cryptic
    if (a->is_deinflection ^ b->is_deinflection) {
        inda = a->is_deinflection;
        indb = b->is_deinflection;
    } else if (s8equals(a->dictname, b->dictname)) {
        inda = a->frequency == 0 ? INT_MAX : a->frequency;
        indb = b->frequency == 0 ? INT_MAX : b->frequency;
    } else {
        inda = indexof((char *)a->dictname.s, cfg.general.dictSortOrder);
        indb = indexof((char *)b->dictname.s, cfg.general.dictSortOrder);
    }

    return inda < indb ? -1 : inda == indb ? 0 : 1;
}

Dict _nonnull_ create_dictionary(s8 *word, Config config) {
    if (config.general.nukeWhitespaceLookup)
        nuke_whitespace(word);

    Dict dict = lookup(word, config);

    if (config.general.sortDictEntries)
        dictSort(dict, dictentry_comparer);

    return dict;
}

/* ---------------- Anki related ----------------- */
static s8 add_bold_tags_around_word(s8 sent, s8 word) {
    return enclose_word_in_s8_with(sent, word, S("<b>"), S("</b>"));
}

static s8 create_furigana(s8 kanji, s8 reading) {
    return (!kanji.len && !reading.len) ? S("")
           // : !reading.len               ? S("") // Don't even try
           : s8equals(kanji, reading) ? s8dup(reading)
                                      : concat(kanji, S("["), reading,
                                               S("]")); // TODO: Obviously not
    // enough if kanji
    // contains hiragana
}

static s8 get_sentence(void) {
#ifdef CLIPBOARD
    if (cfg.anki.copySentence) {
        msg("Please select the context.");
        s8 clip = fromcstr_(get_next_clipboard());
        if (cfg.anki.nukeWhitespaceSentence)
            nuke_whitespace(&clip);
        return clip;
    } else
#endif
    {
        return (s8){0};
    }
}

static void _nonnull_ fill_entries(possible_entries_s *pe, s8 lookup, dictentry const de) {
    pe->copiedsent = get_sentence();
    pe->boldsent = add_bold_tags_around_word(pe->copiedsent, lookup);

    pe->dictdefinition = de.definition;
    pe->dictkanji = de.kanji.len > 0 ? de.kanji : de.reading;
    pe->dictreading = de.reading;
    pe->furigana = create_furigana(de.kanji, de.reading);
    pe->dictname = de.dictname;
}

static s8 map_entry(s8 lookup, dictentry de, int i) {
    // A safer way would be switching to strings, but I feel like that's
    // not very practical to configure
    // return i == 0   ? S("")
    //        : i == 2 ? p.copiedsent
    //        : i == 3 ? p.boldsent
    //        : i == 4 ? p.dictkanji
    //        : i == 5 ? p.dictreading
    //        : i == 6 ? p.dictdefinition
    //        : i == 7 ? p.furigana
    //        : i == 8 ? p.windowname
    //        : i == 9 ? p.dictname
    //                 : S("");

    switch (i) {
        case 0:
            return S("");
        case 1:
            return lookup;
        case 2:
            return get_sentence();
        case 3:
            return add_bold_tags_around_word(get_sentence(), lookup);
        case 4:
            return de.kanji;
        case 5:
            return de.reading;
        case 6:
            return de.definition;
        case 7:
            return create_furigana(de.kanji, de.reading);
        case 8:
            return focused_window_title;
        case 9:
            return de.dictname;
        default:
            err("Anki field mapping number %i is out of bounds.", i);
            return S("");
    }
}

static ankicard prepare_ankicard(s8 lookup, dictentry de, Config config) {

    // possible_entries_s p = {.windowname = focused_window_title};
    // fill_entries(&p, lookup, de);

    char **fieldentries = new (char *, config.anki.numFields);
    for (size_t i = 0; i < config.anki.numFields; i++) {
        fieldentries[i] = (char *)map_entry(lookup, de, config.anki.fieldMapping[i]).s;
    }

    return (ankicard){.deck = config.anki.deck,
                      .notetype = config.anki.notetype,
                      .num_fields = config.anki.numFields,
                      .fieldnames = config.anki.fieldnames,
                      .fieldentries = fieldentries};
}

static void send_ankicard(ankicard ac) {
    char *error = NULL;
    ac_addNote(ac, &error);
    if (error) {
        err("Error adding card: %s", error);
        free(error);
    } else
        msg("Successfully added card.");
}

void create_ankicard(s8 lookup, dictentry de, Config config) {
    ankicard ac = prepare_ankicard(lookup, de, config);
    send_ankicard(ac);
    free(ac.fieldentries);
}
/* ----------------- End Anki related ------------------- */

static void copy_default_database_to(char *path) {
    const char *default_db_loc = NULL;
    for (size_t i = 0; i < arrlen(DEFAULT_DATABASE_LOCATIONS); i++) {
        if (check_file_exists(DEFAULT_DATABASE_LOCATIONS[i])) {
            default_db_loc = DEFAULT_DATABASE_LOCATIONS[i];
            break;
        }
    }
    die_on(default_db_loc == NULL,
           "Could not access the default database either. You need to create your own with"
           "dictpopup-create or download data.mdb from the repository.");

    createdir(path);
    _drop_(frees8) s8 dbpath = buildpath(fromcstr_(path), S("data.mdb"));
    file_copy_sync(default_db_loc, (char *)dbpath.s);
}

void dictpopup_init(void) {
    setlocale(LC_ALL, "");
    read_user_settings(POSSIBLE_ENTRIES_S_NMEMB);

    if (!db_check_exists(fromcstr_(cfg.general.dbDir))) {
        msg("No database found. We recommend creating your own database with dictpopup-create, but "
            "copying default dictionary for now..");
        copy_default_database_to(cfg.general.dbDir);
    }

    focused_window_title = get_windowname();
}
