#include <cgreen/cgreen.h>
#include <glib.h>

#include "yomichan_parser.c"

TestSuite *yomichan_parser_tests(void);

Describe(Parser);
BeforeEach(Parser) {
}
AfterEach(Parser) {
}

static bool frequency_entries_equal(freqentry a, freqentry b) {
    return s8equals(a.word, b.word) && s8equals(a.reading, b.reading) && a.frequency == b.frequency;
}

static bool dict_entries_equal(dictentry a, dictentry b) {
    return s8equals(a.kanji, b.kanji) && s8equals(a.reading, b.reading) &&
           s8equals(a.definition, b.definition) && s8equals(a.dictname, b.dictname) &&
           a.frequency == b.frequency;
}

static _nonnull_ void store_freqentry(freqentry *dest, freqentry fe) {
    static bool called = false;
    if (called) {
        puts("ERROR: store_freqentry was called twice");
        abort();
    }
    *dest = freqentry_dup(fe);
    called = true;
}

static _nonnull_ void store_dictentry(dictentry *dest, dictentry fe) {
    static bool called = false;
    if (called) {
        puts("ERROR: store_dictentry was called twice");
        abort();
    }
    *dest = dictentry_dup(fe);
    called = true;
}

#define CHECK_NR(n)                                                                                \
    do {                                                                                           \
        s8 toparse = {0};                                                                          \
        g_file_get_contents("../tests/files/testentries/" #n "_entry", (char **)&toparse.s,        \
                            (gsize *)&toparse.len, NULL);                                          \
                                                                                                   \
        dictentry parsed = {0};                                                                    \
        parse_yomichan_dictentries_from_buffer(                                                    \
            toparse, S("Test"), (void (*)(void *, dictentry))store_dictentry, &parsed);            \
                                                                                                   \
        s8 expected = {0};                                                                         \
        g_file_get_contents("../tests/files/testentries/" #n "_expected", (char **)&expected.s,    \
                            (gsize *)&expected.len, NULL);                                         \
        g_strchomp((char *)expected.s);                                                            \
        assert_that(parsed.definition.s, is_equal_to_string((char *)expected.s));                  \
                                                                                                   \
        g_free(toparse.s);                                                                         \
        g_free(expected.s);                                                                        \
        dictentry_free(&parsed);                                                                   \
    } while (0)

Ensure(Parser, correctly_parses_structured_content1) {
    CHECK_NR(1);
}

Ensure(Parser, correctly_parses_structured_content2) {
    CHECK_NR(2);
}

Ensure(Parser, skips_furigana) {
    CHECK_NR(3);
}

Ensure(Parser, includes_newlines_between_definitions1) {
    CHECK_NR(4);
}

Ensure(Parser, includes_newlines_between_definitions2) {
    CHECK_NR(5);
}

Ensure(Parser, handles_tag_after_content) {
    CHECK_NR(6);
}

Ensure(Parser, correctly_parses_nested_lists) {
    CHECK_NR(7);
}

Ensure(Parser, correctly_parses_frequency_entry_with_reading) {
    const s8 toparse = S("[[\"糞\", \"freq\", {\"reading\": \"くそ\", \"frequency\": 9788}]]");

    freqentry parsed = {0};
    parse_yomichan_freqentries_from_buffer(toparse, (void (*)(void *, freqentry))store_freqentry,
                                           &parsed);

    freqentry expected = {.word = S("糞"), .reading = S("くそ"), .frequency = 9788};
    assert_that(parsed.word.s, is_equal_to_string((char *)expected.word.s));
    assert_that(parsed.reading.s, is_equal_to_string((char *)expected.reading.s));
    assert_that(parsed.frequency, is_equal_to(expected.frequency));

    freqentry_free(&parsed);
}

Ensure(Parser, correctly_parses_frequency_entry_without_reading) {
    const s8 toparse = S("[[\"私たち\", \"freq\", 581]]");

    freqentry parsed = {0};
    parse_yomichan_freqentries_from_buffer(toparse, (void (*)(void *, freqentry))store_freqentry,
                                           &parsed);

    freqentry expected = {.word = S("私たち"), .reading = (s8){0}, .frequency = 581};
    assert_that(parsed.word.s, is_equal_to_string((char *)expected.word.s));
    assert_that(parsed.reading.s, is_equal_to_string((char *)expected.reading.s));
    assert_that(parsed.frequency, is_equal_to(expected.frequency));

    freqentry_free(&parsed);
}

Ensure(Parser, correctly_extracts_dictionary_name) {
    const s8 buffer = S("{"
    "\"title\": \"Test dictionary\","
    "\"sequenced\": true,"
    "\"format\": 3,"
    "\"revision\": \"2024-05-04\","
    "\"attribution\": \"\""
     "}");

    _drop_(frees8) s8 dictname = extract_dictname_from_buffer(buffer);
    assert_that(dictname.s, is_equal_to_string("Test dictionary"));
}

TestSuite *yomichan_parser_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, Parser, skips_furigana);
    add_test_with_context(suite, Parser, includes_newlines_between_definitions1);
    add_test_with_context(suite, Parser, includes_newlines_between_definitions2);
    add_test_with_context(suite, Parser, correctly_parses_structured_content1);
    add_test_with_context(suite, Parser, correctly_parses_structured_content2);
    add_test_with_context(suite, Parser, correctly_parses_frequency_entry_with_reading);
    add_test_with_context(suite, Parser, correctly_parses_frequency_entry_without_reading);
    add_test_with_context(suite, Parser, correctly_extracts_dictionary_name);
    // add_test_with_context(suite, Parser, handles_tag_after_content);
    // add_test_with_context(suite, Parser, correctly_parses_nested_lists);
    return suite;
}