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

Ensure(Parser, skips_furigana) {
    json_stream s[1];

    const char *str_to_parse =
        "[\"々\", \"のま\", \"\", \"\", 0, [{\"type\": \"structured-content\", \"content\": [{\"tag\": \"div\", \"content\": [{\"tag\": \"span\", \"style\": {\"verticalAlign\": \"text-bottom\", \"marginRight\": 0.25}, \"data\": {\"code\": \"unc\"}, \"content\": {\"tag\": \"img\", \"height\": 1.2, \"width\": 3.89, \"sizeUnits\": \"em\", \"appearance\": \"auto\", \"title\": \"unclassified\", \"collapsible\": false, \"collapsed\": false, \"background\": false, \"path\": \"jitendex/unclass.svg\"}}, {\"tag\": \"div\", \"content\": {\"tag\": \"ul\", \"style\": {\"marginBottom\": 0.5}, \"content\": [{\"tag\": \"li\", \"style\": {\"listStyleType\": \"square\", \"marginTop\": 0.5}, \"data\": {\"content\": \"info-gloss\"}, \"content\": \"kanji repetition mark\"}, {\"tag\": \"li\", \"style\": {\"fontSize\": \"120%\", \"listStyleType\": \"\\\"➡ \\\"\"}, \"data\": {\"content\": \"xref\"}, \"content\": [{\"tag\": \"span\", \"style\": {\"marginRight\": 0.25}, \"content\": \"see:\"}, {\"tag\": \"a\", \"lang\": \"ja\", \"href\": \"?query=同の字点&wildcards=off\", \"content\": [{\"tag\": \"ruby\", \"content\": [\"同\", {\"tag\": \"rt\", \"content\": \"どう\"}]}, \"の\", {\"tag\": \"ruby\", \"content\": [\"字\", {\"tag\": \"rt\", \"content\": \"じ\"}]}, {\"tag\": \"ruby\", \"content\": [\"点\", {\"tag\": \"rt\", \"content\": \"てん\"}]}]}]}, {\"tag\": \"li\", \"style\": {\"fontSize\": \"70%\", \"listStyleType\": \"none\"}, \"data\": {\"content\": \"xref-glossary\"}, \"content\": \"kanji iteration mark\"}]}}]}, {\"tag\": \"div\", \"data\": {\"content\": \"forms\"}, \"content\": [{\"tag\": \"span\", \"style\": {\"verticalAlign\": \"text-bottom\", \"marginRight\": 0.25}, \"content\": {\"tag\": \"img\", \"height\": 1.2, \"width\": 2.97, \"sizeUnits\": \"em\", \"appearance\": \"auto\", \"title\": \"spelling and reading variants\", \"collapsible\": false, \"collapsed\": false, \"background\": false, \"path\": \"jitendex/forms.svg\"}}, {\"tag\": \"ul\", \"content\": [{\"tag\": \"li\", \"content\": \"々\"}, {\"tag\": \"li\", \"content\": \"ノマ\"}]}]}]}], 1000060, \"\"]";
    json_open_string(s, str_to_parse);
    json_next(s);

    dictentry parsed = parse_dictionary_entry(s);

    const char *expected =
        "▪ kanji repetition mark\n➡  see:同の字点\n  kanji iteration mark\n• 々\n• ノマ";
    assert_that(parsed.definition.s, is_equal_to_string(expected));

    dictentry_free(&parsed);
    json_close(s);
}

Ensure(Parser, correctly_parses_structured_content1) {
    json_stream s[1];

    char *str_to_parse = NULL;
    g_file_get_contents("../tests/files/testentries/1_entry", &str_to_parse, NULL, NULL);
    json_open_string(s, str_to_parse);
    json_next(s);

    dictentry parsed = parse_dictionary_entry(s);

    char *expected = NULL;
    g_file_get_contents("../tests/files/testentries/1_expected", &expected, NULL, NULL);
    assert_that(parsed.definition.s, is_equal_to_string(expected));

    g_free(str_to_parse);
    g_free(expected);
    dictentry_free(&parsed);
    json_close(s);
}

Ensure(Parser, correctly_parses_structured_content2) {
    json_stream s[1];

    char *str_to_parse = NULL;
    g_file_get_contents("../tests/files/testentries/2_entry", &str_to_parse, NULL, NULL);
    json_open_string(s, str_to_parse);
    json_next(s);

    dictentry parsed = parse_dictionary_entry(s);

    char *expected = NULL;
    g_file_get_contents("../tests/files/testentries/2_expected", &expected, NULL, NULL);
    assert_that(parsed.definition.s, is_equal_to_string(expected));

    g_free(str_to_parse);
    g_free(expected);
    dictentry_free(&parsed);
    json_close(s);
}

Ensure(Parser, correctly_parses_frequency_entry) {
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

TestSuite *parser_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, Parser, skips_furigana);
    add_test_with_context(suite, Parser, correctly_parses_structured_content1);
    add_test_with_context(suite, Parser, correctly_parses_structured_content2);
    add_test_with_context(suite, Parser, correctly_parses_frequency_entry);
    return suite;
}