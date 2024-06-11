#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>
#include <glib.h>

#include "yomichan_parser.c"

#define DICTENTRIES_DIR "../tests/files/dictionary_entries"

TestSuite *yomichan_parser_tests(void);

Describe(Parser);
BeforeEach(Parser) {
}
AfterEach(Parser) {
}

static void foreach_freqentry(void *userdata, freqentry fe) {
    mock(fe.frequency, fe.reading.s, fe.word.s);
}

static _nonnull_ void foreach_dictentry(void *userdata, dictentry de) {
    mock(de.definition.s, de.dictname.s, de.kanji.s, de.reading.s);
}

#define CHECK_NR(n)                                                                                \
    do {                                                                                           \
        s8 toparse = {0};                                                                          \
        g_file_get_contents(DICTENTRIES_DIR "/" #n "_entry", (char **)&toparse.s,        \
                            (gsize *)&toparse.len, NULL);                                          \
                                                                                                   \
        s8 expected = {0};                                                                         \
        g_file_get_contents(DICTENTRIES_DIR "/" #n "_expected", (char **)&expected.s,    \
                            (gsize *)&expected.len, NULL);                                         \
        g_strchomp((char *)expected.s);                                                            \
                                                                                                   \
        expect(foreach_dictentry,                                             \
               when(de.definition.s, is_equal_to_string((char *)expected.s)),                      \
               when(de.dictname.s, is_equal_to_string("Test")));                                   \
        parse_yomichan_dictentries_from_buffer(toparse, S("Test"), &foreach_dictentry, NULL);      \
                                                                                                   \
        g_free(toparse.s);                                                                         \
        g_free(expected.s);                                                                        \
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

Ensure(Parser, handles_tag_after_content1) {
    CHECK_NR(6);
}

Ensure(Parser, handles_tag_after_content2) {
    CHECK_NR(7);
}

Ensure(Parser, correctly_parses_nested_lists) {
    CHECK_NR(8);
}

Ensure(Parser, correctly_parses_frequency_entry_with_reading) {
    const s8 toparse = S("[[\"糞\", \"freq\", {\"reading\": \"くそ\", \"frequency\": 9788}]]");

    freqentry expected = {.word = S("糞"), .reading = S("くそ"), .frequency = 9788};
    expect(foreach_freqentry,
           when(fe.word.s, is_equal_to_string((char *)expected.word.s)),
           when(fe.reading.s, is_equal_to_string((char *)expected.reading.s)),
           when(fe.frequency, is_equal_to(expected.frequency)));
    parse_yomichan_freqentries_from_buffer(toparse, &foreach_freqentry, NULL);
}

Ensure(Parser, correctly_parses_frequency_entry_without_reading) {
    const s8 toparse = S("[[\"私たち\", \"freq\", 581]]");

    freqentry expected = {.word = S("私たち"), .reading = (s8){0}, .frequency = 581};

    expect(foreach_freqentry,
           when(fe.word.s, is_equal_to_string((char *)expected.word.s)),
           when(fe.reading.s, is_equal_to_string((char *)expected.reading.s)),
           when(fe.frequency, is_equal_to(expected.frequency)));
    parse_yomichan_freqentries_from_buffer(toparse, &foreach_freqentry, NULL);
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
    add_test_with_context(suite, Parser, handles_tag_after_content1);
    add_test_with_context(suite, Parser, handles_tag_after_content2);
    // add_test_with_context(suite, Parser, correctly_parses_nested_lists);
    return suite;
}