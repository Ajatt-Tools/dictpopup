#include <cgreen/cgreen.h>

#include "ankiconnectc.h"
#include "db_mock.c"
#include "dictpopup.c"
#include "utils/util.h"

Config cfg = {0};
void read_user_settings(int fieldmapping_max) {
    return;
}

s8 get_windowname(void) {
    return (s8){0};
}

void file_copy_sync(const char *source_path, const char *dest_path) {
    return;
}

void createdir(char *dirpath) {
    return;
}

bool check_file_exists(const char *fn) {
    return true;
}

TestSuite *dictpopup_tests(void);

Describe(DictPopup);
BeforeEach(DictPopup) {
}
AfterEach(DictPopup) {
}

Ensure(DictPopup, properly_prepares_ankicard) {
    // s8 sent = S("白鯨同様、世界中が被害を被っている。騎士団も長く辛酸を味わわされてきた相手だ");
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

    ankicard ac = prepare_ankicard(lookup, de, config);

    assert_that(ac.fieldentries[0], is_equal_to_string(0));
    assert_that(ac.fieldentries[1], is_equal_to_string("辛酸"));
    assert_that(ac.fieldentries[2], is_equal_to_string("辛酸[しんさん]"));
    assert_that(ac.fieldentries[3], is_equal_to_string("つらい思い。苦しみ。"));
    assert_that(ac.fieldentries[4], is_equal_to_string(0));

    free(ac.fieldentries);
}

TestSuite *dictpopup_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, DictPopup, properly_prepares_ankicard);
    return suite;
}
