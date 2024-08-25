#include <cgreen/cgreen.h>
#include <utils/str.h>

TestSuite *s8_tests(void);

Describe(enclose_word_in_string);
BeforeEach(enclose_word_in_string) {
}
AfterEach(enclose_word_in_string) {
}

Ensure(enclose_word_in_string, handles_all_null_input) {
    _drop_(frees8) s8 bolded = enclose_word_in_s8_with(fromcstr_(0), fromcstr_(0), fromcstr_(0), fromcstr_(0));

    assert_that(bolded.s, is_equal_to_string(0));
}

Ensure(enclose_word_in_string, handles_null_sentence_rest_non_null) {
    _drop_(frees8) s8 bolded = enclose_word_in_s8_with(fromcstr_(0), S("麒麟"), S("abc"), S("efg"));

    assert_that(bolded.s, is_equal_to_string(0));
}

Ensure(enclose_word_in_string, properly_encloses_string_with_one_occurence) {
    const s8 sentence = S("この本はとても面白いです。");
    const s8 word = S("本");

    _drop_(frees8) s8 bolded = enclose_word_in_s8_with(sentence, word, S("<b>"), S("</b>"));

    assert_that(bolded.s, is_equal_to_string("この<b>本</b>はとても面白いです。"));
}

Ensure(enclose_word_in_string, properly_encloses_string_with_four_occurences) {
    const s8 sentence =
        S("相手の態度で、こちらの態度を変える。相手の態度で、こちらの態度を変える。");
    const s8 word = S("態度");

    _drop_(frees8) s8 bolded = enclose_word_in_s8_with(sentence, word, S("<b>"), S("</b>"));

    assert_that(bolded.s, is_equal_to_string("相手の<b>態度</b>で、こちらの<b>態度</b>を変える。"
                                             "相手の<b>態度</b>で、こちらの<b>態度</b>を変える。"));
}

TestSuite *s8_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, enclose_word_in_string, properly_encloses_string_with_one_occurence);
    add_test_with_context(suite, enclose_word_in_string, properly_encloses_string_with_four_occurences);
    add_test_with_context(suite, enclose_word_in_string, handles_all_null_input);
    add_test_with_context(suite, enclose_word_in_string, handles_null_sentence_rest_non_null);
    return suite;
}
