#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "ankiconnectc.h"
#include "db.h"
#include "deinflector.h"
#include "dictpopup.h"
#include "messages.h"
#include "settings.h"
#include "utils/utf8.h"
#include "utils/util.h"

#include "platformdep/file_operations.h"
#include "platformdep/windowtitle.h"
#include "platformdep/file_paths.h"
#ifdef CLIPBOARD
    #include "platformdep/clipboard.h"
#endif

// This only applies for substring search
// 60 are 20 Japanese characters
#define MAX_LOOKUP_LEN 60 // in bytes

s8 focused_window_title = {0};

// TODO: Cleaner (and non-null terminated) implementation
static void nuke_whitespace(s8 *z) {
    substrremove((char *)z->s, S("\n"));
    substrremove((char *)z->s, S("\t"));
    substrremove((char *)z->s, S(" "));
    substrremove((char *)z->s, S("　"));

    *z = fromcstr_((char *)z->s);
}

static s8 add_bold_tags_around_word(s8 sent, s8 word) {
    return enclose_word_in_s8_with(sent, word, S("<b>"), S("</b>"));
}

static s8 create_furigana(s8 kanji, s8 reading) {
    return (!kanji.len && !reading.len) ? S("")
           /* : !reading.len ? S("") // Don't try */
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
        s8 clip = get_next_clipboard();
        if (cfg.anki.nukeWhitespaceSentence)
            nuke_whitespace(&clip);
        return clip;
    } else
#endif
    {
        return (s8){0};
    }
}

static void fill_entries(possible_entries_s pe[static 1], s8 lookup, dictentry const de) {
    pe->copiedsent = get_sentence();
    pe->boldsent = add_bold_tags_around_word(pe->copiedsent, lookup);

    pe->dictdefinition = de.definition;
    pe->dictkanji = de.kanji.len > 0 ? de.kanji : de.reading;
    pe->dictreading = de.reading;
    pe->furigana = create_furigana(de.kanji, de.reading);
    pe->dictname = de.dictname;
}

static void _nonnull_ add_deinflections_to_dict(const database_t *db, s8 word,
                                                dictentry *dict[static 1]) {
    _drop_(frees8buffer) s8 *deinfs_b = deinflect(word);

    for (size_t i = 0; i < buf_size(deinfs_b); i++)
        db_get_dictents(db, deinfs_b[i], dict);
}

static dictentry _nonnull_ *lookup(const database_t *db, s8 word) {
    dictentry *dict = NULL;
    db_get_dictents(db, word, &dict);
    add_deinflections_to_dict(db, word, &dict);
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

static int _nonnull_ dictentry_comparer(void const *voida, void const *voidb) {
    dictentry a = *(dictentry *)voida;
    dictentry b = *(dictentry *)voidb;

    int inda, indb;
    if (s8equals(a.dictname, b.dictname)) {
        inda = a.frequency == 0 ? INT_MAX : a.frequency;
        indb = b.frequency == 0 ? INT_MAX : b.frequency;
    } else {
        inda = indexof((char *)a.dictname.s, cfg.general.dictSortOrder);
        indb = indexof((char *)b.dictname.s, cfg.general.dictSortOrder);
    }

    return inda < indb ? -1 : inda == indb ? 0 : 1;
}

static void sort_dictentries(dictentry *dict) {
    if (dict)
        qsort(dict, dictlen(dict), sizeof(dictentry), dictentry_comparer);
}

static dictentry *lookup_first_matching_prefix(s8 *word, database_t *db) {
    dictentry *dict = NULL;

    int first_chr_len = utf8_chr_len(word->s);
    while (dict == NULL && word->len > first_chr_len) {
        *word = striputf8chr(*word);
        word->s[word->len] = '\0';        // TODO: Remove necessity for this
        if (word->len < MAX_LOOKUP_LEN) { // Don't waste time looking up huge strings
            dict = lookup(db, *word);
        }
    }

    return dict;
}

dictentry *create_dictionary(s8 *word) {
    assert(word->len);
    dictentry *dict = NULL;

    if (cfg.general.nukeWhitespaceLookup)
        nuke_whitespace(word);

    _drop_(db_close) database_t *db = db_open(cfg.general.dbpth, true);

    dict = lookup(db, *word);

    if (dict == NULL && cfg.general.mecab) {
        _drop_(frees8) s8 hira = kanji2hira(*word);
        dict = lookup(db, hira);
    }

    if (dict == NULL && cfg.general.substringSearch) {
        dict = lookup_first_matching_prefix(word, db);
    }

    if (cfg.general.sort)
        sort_dictentries(dict);

    return dict;
}

static s8 map_entry(possible_entries_s p, int i) {
    // A safer way would be switching to strings, but I feel like that's
    // not very practical to configure
    return i == 0   ? S("")
           : i == 2 ? p.copiedsent
           : i == 3 ? p.boldsent
           : i == 4 ? p.dictkanji
           : i == 5 ? p.dictreading
           : i == 6 ? p.dictdefinition
           : i == 7 ? p.furigana
           : i == 8 ? p.windowname
           : i == 9 ? p.dictname
                    : S("");
}

static ankicard prepare_ankicard(s8 lookup, dictentry de, Config config) {

    possible_entries_s p = {.windowname = focused_window_title};
    fill_entries(&p, lookup, de);

    char **fieldentries = new (char *, config.anki.numFields);
    for (size_t i = 0; i < config.anki.numFields; i++) {
        fieldentries[i] = (char *)map_entry(p, config.anki.fieldMapping[i]).s;
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

static void copy_default_database(char *dbdir) {
    const char *default_db_loc = NULL;
    for(size_t i = 0; i < sizeof(DEFAULT_DATABASE_LOCATIONS); i++) {
        if(check_file_exists(DEFAULT_DATABASE_LOCATIONS[i]))
            default_db_loc = DEFAULT_DATABASE_LOCATIONS[i];
    }
    die_on(!default_db_loc,
           "Could not access the default database either. You need to create your own with"
           "dictpopup-create or download data.mdb from the repository.");

    createdir(dbdir);
    _drop_(frees8) s8 dbpath = buildpath(fromcstr_(dbdir), S("data.mdb"));
    file_copy_sync(default_db_loc, (char *)dbpath.s);
}

void dictpopup_init(void) {
    setlocale(LC_ALL, "");
    read_user_settings(POSSIBLE_ENTRIES_S_NMEMB);

    if (!db_check_exists(fromcstr_(cfg.general.dbpth))) {
        msg("No database found. We recommend creating your own database with dictpopup-create, but "
            "copying default dictionary for now..");
        copy_default_database(cfg.general.dbpth);
    }

    focused_window_title = get_windowname();
}
