#include <cgreen/cgreen.h>

#include "ankiconnectc.h"
#include "db_mock.c"
#include "dictionary_lookup.c"
#include "utils/util.h"

Config cfg = {0};
void read_user_settings(int fieldmapping_max) {
    return;
}

s8 get_windowname(void) {
    return (s8){0};
}

void file_copy_sync(const char *source_path, const char *dest_path) {
    mock(source_path, dest_path);
    return;
}

void createdir(char *dirpath) {
    return;
}

bool check_file_exists(const char *fn) {
    return (bool)mock(fn);
}

const char *get_user_data_dir(void) {
    return "/home/user/.local/share";
}

TestSuite *dictpopup_tests(void);

Describe(DictPopup);
BeforeEach(DictPopup) {
}
AfterEach(DictPopup) {
}

Ensure(DictPopup, properly_prepares_ankicard) {
    s8 sent = S("白鯨同様、世界中が被害を被っている。騎士団も長く辛酸を味わわされてきた相手だ");
    s8 lookup = S("辛酸");
    dictentry de = (dictentry){.dictname = S("明鏡国語辞典　第二版"),
                               .kanji = S("辛酸"),
                               .reading = S("しんさん"),
                               .definition = S("つらい思い。苦しみ。"),
                               .frequency = 12831293};
    char *fieldNames[] = {"SentKanji", "VocabKanji", "VocabFurigana", "VocabDef", "Notes"};
    u32 fieldMapping[] = {3, 4, 7, 6, 8};
    Config config = {.anki = {.copySentence = false,
                              .numFields = arrlen(fieldMapping),
                              .fieldMapping = fieldMapping,
                              .fieldnames = fieldNames,
                              .deck = "Japanese",
                              .notetype = "Japanese Sentences"}};
    cfg.anki.copySentence = true;

    ankicard ac = prepare_ankicard(lookup, sent, de, config);

    assert_that(
        ac.fieldentries[0],
        is_equal_to_string(
            "白鯨同様、世界中が被害を被っている。騎士団も長く<b>辛酸</b>を味わわされてきた相手だ"));
    assert_that(ac.fieldentries[1], is_equal_to_string("辛酸"));
    assert_that(ac.fieldentries[2], is_equal_to_string("辛酸[しんさん]"));
    assert_that(ac.fieldentries[3], is_equal_to_string("つらい思い。苦しみ。"));
    assert_that(ac.fieldentries[4], is_equal_to_string(0));

    free(ac.fieldentries);
}

Ensure(DictPopup, looks_up_substrings_as_expected) {
    _drop_(frees8) s8 lookup = s8dup(S("世界からはいつの間にか風の音が、虫の鳴き声が消失していた"));
    _drop_(db_close) database_t *db = db_open("dummy", true);

    dictentry *dict = lookup_first_matching_prefix(&lookup, db);

    assert_that(dict[0].kanji.s, is_equal_to_string("世界"));

    buf_free(dict);
}

Ensure(DictPopup, copies_default_database_if_no_database_exists) {
    _drop_(free) s8 dbdir = buildpath(fromcstr_((char *)get_user_data_dir()), S("dictpopup"));
    _drop_(free) s8 dbfile = buildpath(dbdir, S("data.mdb"));

    always_expect(check_file_exists, will_return(true));
    always_expect(db_check_exists, will_return(false));

    expect(file_copy_sync, when(source_path, is_equal_to_string(DEFAULT_DATABASE_LOCATIONS[0])),
           when(dest_path, is_equal_to_string((char *)dbfile.s)));

    // expect(read_user_settings, )
    // cfg.general.dbDir = (char*)dbdir.s;
    dictpopup_init();
}

Ensure(DictPopup, sorts_direct_matches_first) {
    //TODO
    assert(true);
}

TestSuite *dictpopup_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, DictPopup, properly_prepares_ankicard);
    add_test_with_context(suite, DictPopup, looks_up_substrings_as_expected);
    add_test_with_context(suite, DictPopup, sorts_direct_matches_first);
    // add_test_with_context(suite, DictPopup, copies_default_database_if_no_database_exists);
    return suite;
}
