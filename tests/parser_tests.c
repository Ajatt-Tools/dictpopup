#include "dictpopup_create.c"
#include <cgreen/cgreen.h>
#include <glib.h>

TestSuite *parser_tests(void);

Describe(Parser);
BeforeEach(Parser) {
}
AfterEach(Parser) {
}

static bool frequency_entries_equal(frequency_entry a, frequency_entry b) {
    return s8equals(a.word, b.word) && s8equals(a.reading, b.reading) && a.frequency == b.frequency;
}

static bool dict_entries_equal(dictentry a, dictentry b) {
    return s8equals(a.kanji, b.kanji) && s8equals(a.reading, b.reading) &&
           s8equals(a.definition, b.definition) && s8equals(a.dictname, b.dictname) &&
           a.frequency == b.frequency;
}

#define CHECK_NR(n)                                                                                \
    do {                                                                                           \
        json_stream s[1];                                                                          \
                                                                                                   \
        char *str_to_parse = NULL;                                                                 \
        g_file_get_contents("../tests/files/testentries/" #n "_entry", &str_to_parse, NULL, NULL); \
        json_open_string(s, str_to_parse);                                                         \
        json_next(s);                                                                              \
        json_next(s);                                                                              \
                                                                                                   \
        dictentry parsed = parse_dictionary_entry(s);                                              \
                                                                                                   \
        char *expected = NULL;                                                                     \
        g_file_get_contents("../tests/files/testentries/" #n "_expected", &expected, NULL, NULL);  \
        g_strchomp(expected);                                                                      \
        assert_that(parsed.definition.s, is_equal_to_string(expected));                            \
                                                                                                   \
        g_free(str_to_parse);                                                                      \
        g_free(expected);                                                                          \
        dictentry_free(&parsed);                                                                   \
        json_close(s);                                                                             \
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

Ensure(Parser, correctly_parses_frequency_entry_with_reading) {
    json_stream s[1];

    const char *str_to_parse = "[\"糞\", \"freq\", {\"reading\": \"くそ\", \"frequency\": 9788}]";
    json_open_string(s, str_to_parse);
    json_next(s);

    frequency_entry parsed = parse_frequency_entry(s);

    frequency_entry expected = {.word = S("糞"), .reading = S("くそ"), .frequency = 9788};
    bool equals = frequency_entries_equal(parsed, expected);
    assert_that(equals, is_true);

    frequency_entry_free(&parsed);
    json_close(s);
}

Ensure(Parser, correctly_parses_frequency_entry_without_reading) {
    json_stream s[1];

    const char *str_to_parse = "[\"私たち\", \"freq\", 581]";
    json_open_string(s, str_to_parse);
    json_next(s);

    frequency_entry parsed = parse_frequency_entry(s);

    frequency_entry expected = {.word = S("私たち"), .reading = (s8){0}, .frequency = 581};
    bool equals = frequency_entries_equal(parsed, expected);
    assert_that(equals, is_true);

    frequency_entry_free(&parsed);
    json_close(s);
}

Ensure(Parser, handles_tag_after_content) {
    CHECK_NR(6);
}

TestSuite *parser_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, Parser, skips_furigana);
    add_test_with_context(suite, Parser, includes_newlines_between_definitions1);
    add_test_with_context(suite, Parser, includes_newlines_between_definitions2);
    add_test_with_context(suite, Parser, correctly_parses_structured_content1);
    add_test_with_context(suite, Parser, correctly_parses_structured_content2);
    add_test_with_context(suite, Parser, correctly_parses_frequency_entry_with_reading);
    add_test_with_context(suite, Parser, correctly_parses_frequency_entry_without_reading);
    // add_test_with_context(suite, Parser, handles_tag_after_content);
    return suite;
}