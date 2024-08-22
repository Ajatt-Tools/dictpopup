#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>
#include <glib.h>

#include "anki.c"

TestSuite *anki_card_creator_tests(void);

Describe(AnkiCardCreator);
BeforeEach(AnkiCardCreator) {
}
AfterEach(AnkiCardCreator) {
}

static void ankicard_assert_equal(AnkiCard actual, AnkiCard expected) {
    assert_that(actual.deck, is_equal_to_string(expected.deck));
    assert_that(actual.notetype, is_equal_to_string(expected.notetype));
    assert_that(actual.num_fields, is_equal_to(expected.num_fields));
    for (size_t i = 0; i < actual.num_fields; i++) {
        assert_that(actual.fieldnames[i], is_equal_to_string(expected.fieldnames[i]));
        assert_that(actual.fieldentries[i], is_equal_to_string(expected.fieldentries[i]));
    }
}

Ensure(AnkiCardCreator, correctly_formats_furigana_without_reading) {
    _drop_(frees8) s8 dictname = create_furigana(S("初耳"), (s8){0});
    assert_that(dictname.s, is_equal_to_string("初耳"));
}

Ensure(AnkiCardCreator, correctly_formats_furigana_without_kanji) {
    _drop_(frees8) s8 dictname = create_furigana((s8){0}, S("はつみみ"));
    assert_that(dictname.s, is_equal_to_string("はつみみ"));
}

Ensure(AnkiCardCreator, correctly_formats_furigana_full_kanji) {
    _drop_(frees8) s8 dictname = create_furigana(S("初耳"), S("はつみみ"));
    assert_that(dictname.s, is_equal_to_string("初耳[はつみみ]"));
}

// TODO TODO
Ensure(AnkiCardCreator, properly_prepares_full_ankicard) {
    s8 sent = S("白鯨同様、世界中が被害を被っている。騎士団も長く辛酸を味わわされてきた相手だ");
    s8 lookup = S("辛酸");
    Dictentry de = (Dictentry){.dictname = S("明鏡国語辞典　第二版"),
                               .kanji = S("辛酸"),
                               .reading = S("しんさん"),
                               .definition = S("つらい思い。苦しみ。"),
                               .frequency = 12831293};
    char *fieldNames[] = {"SentKanji", "VocabKanji", "VocabFurigana", "VocabDef", "Notes"};
    u32 fieldMapping[] = {DP_ANKI_BOLD_COPIED_SENTENCE, DP_ANKI_DICTIONARY_KANJI, DP_ANKI_FURIGANA, DP_ANKI_DICTIONARY_DEFINITION, DP_ANKI_FOCUSED_WINDOW_NAME};
    AnkiFieldMapping field_mapping = (AnkiFieldMapping) {.field_names = fieldNames, .field_content = fieldMapping, .num_fields = arrlen(fieldMapping)};
    AnkiConfig config = {.fieldmapping = field_mapping,
                         .deck = "Japanese",
                         .notetype = "Japanese Sentences"};

    char *expected_field_entries[] = {"白鯨同様、世界中が被害を被っている。騎士団も長く<b>辛酸</b>を味わわされてきた相手だ", "辛酸", "辛酸[しんさん]", "つらい思い。苦しみ。", ""};
    AnkiCard expected = (AnkiCard) {
        .deck = "Japanese",
        .notetype = "Japanese Sentences",
        .fieldnames = fieldNames,
        .num_fields = 5,
        .fieldentries = expected_field_entries,
    };

    AnkiCard actual = prepare_ankicard(lookup, sent, de, config);

    ankicard_assert_equal(actual, expected);

    free_prepared_ankicard(actual);
}

TestSuite *anki_card_creator_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, AnkiCardCreator, correctly_formats_furigana_without_reading);
    add_test_with_context(suite, AnkiCardCreator, correctly_formats_furigana_without_kanji);
    add_test_with_context(suite, AnkiCardCreator, correctly_formats_furigana_full_kanji);
    add_test_with_context(suite, AnkiCardCreator, properly_prepares_full_ankicard);
    return suite;
}