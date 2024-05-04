#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "ankiconnectc.h"
#include "db.h"
#include "deinflector.h"
#include "dictpopup.h"
#include "messages.h"
#include "platformdep.h"
#include "settings.h"
#include "util.h"

static s8 map_entry(possible_entries_s p, int i) {
    // A safer way would be switching to strings, but I feel like that's
    // not very practical to configure
    return i == 0   ? S("")
           : i == 1 ? p.lookup
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

static s8 add_bold_tags(s8 sent, s8 word) {
    _drop_(frees8) s8 bdword = concat(S("<b>"), word, S("</b>"));

    assert(word.s[word.len] == '\0');

    GString *bdsent = g_string_new_len((gchar *)sent.s, (gssize)sent.len);
    g_string_replace(bdsent, (char *)word.s, (char *)bdword.s, 0);

    s8 ret = {0};
    ret.len = bdsent->len;
    ret.s = (u8 *)g_string_free(bdsent, FALSE);
    return ret;
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

static void fill_entries(possible_entries_s pe[static 1], dictentry const de) {
    if (cfg.anki.copySentence) {
        msg("Please select the context.");
        pe->copiedsent = get_sentence();
        if (cfg.anki.nukeWhitespaceSentence)
            pe->copiedsent = nuke_whitespace(pe->copiedsent);

        pe->boldsent = add_bold_tags(pe->copiedsent, pe->lookup);
    }

    pe->dictdefinition = s8dup(de.definition);
    pe->dictkanji = s8dup(de.kanji.len > 0 ? de.kanji : de.reading);
    pe->dictreading = s8dup(de.reading);
    pe->furigana = create_furigana(de.kanji, de.reading);
    pe->dictname = de.dictname;
}

static void add_deinflections_to_dict(database_t db[static 1], s8 word, dictentry *dict[static 1]) {
    s8 *deinfs_b = deinflect(word);
    for (size_t i = 0; i < buf_size(deinfs_b); i++)
        db_get_dictents(db, deinfs_b[i], dict);
    frees8buffer(&deinfs_b);
}

static dictentry *lookup(database_t db[static 1], s8 word) {
    dbg("Looking up: %.*s", (int)word.len, word.s);

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

static int dictentry_comparer(void const *voida, void const *voidb) {
    assert(voida && voidb);
    dictentry a = *(dictentry *)voida;
    dictentry b = *(dictentry *)voidb;

    int inda = 0, indb = 0;
    if (s8equals(a.dictname, b.dictname)) {
        inda = a.frequency == -1 ? INT_MAX : a.frequency;
        indb = b.frequency == -1 ? INT_MAX : b.frequency;
    } else {
        inda = indexof((char *)a.dictname.s, cfg.general.dictSortOrder);
        indb = indexof((char *)b.dictname.s, cfg.general.dictSortOrder);
    }

    return inda < indb ? -1 : inda == indb ? 0 : 1;
}

dictentry *create_dictionary(dictpopup_t *d) {
    dictentry *dict = NULL;

    database_t db = db_open(cfg.general.dbpth, true);

    // TODO: Fix lookup name clash
    dict = lookup(&db, d->pe.lookup);
    if (!dict && cfg.general.mecab) {
        _drop_(frees8) s8 hira = kanji2hira(d->pe.lookup);
        dict = lookup(&db, hira);
    }
    if (!dict && cfg.general.substringSearch) {
        int first_chr_len = utf8_chr_len(d->pe.lookup.s);
        while (dictlen(dict) == 0 && d->pe.lookup.len > first_chr_len) {
            d->pe.lookup = s8striputf8chr(d->pe.lookup);
            d->pe.lookup.s[d->pe.lookup.len] = '\0'; // TODO: Remove necessity for this
            dict = lookup(&db, d->pe.lookup);
        }
    }

    db_close(&db);

    if (!dict) {
        msg("No dictionary entry found");
        exit(EXIT_FAILURE);
    }

    if (cfg.general.sort)
        qsort(dict, dictlen(dict), sizeof(dictentry), dictentry_comparer);

    return dict;
}

void create_ankicard(dictpopup_t d, dictentry de) {
    possible_entries_s p = d.pe;
    fill_entries(&p, de);

    _drop_(free) char **fieldentries = new (char *, cfg.anki.numFields);
    for (size_t i = 0; i < cfg.anki.numFields; i++)
        fieldentries[i] = (char *)map_entry(p, cfg.anki.fieldMapping[i]).s;

    retval_s ac_resp = ac_addNote((ankicard){.deck = cfg.anki.deck,
                                             .notetype = cfg.anki.notetype,
                                             .num_fields = cfg.anki.numFields,
                                             .fieldnames = cfg.anki.fieldnames,
                                             .fieldentries = fieldentries});

    if (ac_resp.ok)
        msg("Successfully added card.");
    else
        err("Error adding card: %s", ac_resp.data.string);
}

dictpopup_t dictpopup_init(int argc, char **argv) {
    int nextarg = parse_cmd_line_opts(argc, argv);
    read_user_settings(POSSIBLE_ENTRIES_S_NMEMB);

    die_on(!db_check_exists(fromcstr_(cfg.general.dbpth)),
           "Database does not exist. You must create it first with dictpopup-create.");

    possible_entries_s p = {0};

    p.windowname = get_windowname();
    p.lookup = argc - nextarg > 0 ? fromcstr_(argv[nextarg]) : get_selection();

    die_on(!p.lookup.len, "No selection and no argument provided. Exiting..");
    die_on(!g_utf8_validate((char *)p.lookup.s, p.lookup.len, NULL),
           "Lookup is not a valid UTF-8 string");

    if (cfg.general.nukeWhitespaceLookup)
        nuke_whitespace(p.lookup);

    return (dictpopup_t){p};
}
