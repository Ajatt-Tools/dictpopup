#include <cgreen/cgreen.h>
#include <utils/str.h>

TestSuite *s8_tests(void);

Describe(enclose_word_in_string);
BeforeEach(enclose_word_in_string) {
}
AfterEach(enclose_word_in_string) {
}

Ensure(enclose_word_in_string, handles_all_null_input) {
    _drop_(frees8) s8 bolded =
        enclose_word_in_s8_with(fromcstr_(0), fromcstr_(0), fromcstr_(0), fromcstr_(0));

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

Ensure(enclose_word_in_string, properly_encloses_at_end) {
    const s8 sentence = S("事前に昏倒させられたのではない。ただ、一つ解せないことがあって");
    const s8 word = S("解せないことがあって");

    _drop_(frees8) s8 bolded = enclose_word_in_s8_with(sentence, word, S("<b>"), S("</b>"));

    assert_that(bolded.s,
                is_equal_to_string(
                    "事前に昏倒させられたのではない。ただ、一つ<b>解せないことがあって</b>"));
}

Ensure(enclose_word_in_string, properly_encloses_at_start) {
    const s8 sentence = S("事前に昏倒させられたのではない。ただ、一つ解せないことがあって");
    const s8 word = S("事前");

    _drop_(frees8) s8 bolded = enclose_word_in_s8_with(sentence, word, S("<b>"), S("</b>"));

    assert_that(bolded.s,
                is_equal_to_string(
                    "<b>事前</b>に昏倒させられたのではない。ただ、一つ解せないことがあって"));
}

TestSuite *s8_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, enclose_word_in_string, properly_encloses_string_with_one_occurence);
    add_test_with_context(suite, enclose_word_in_string, properly_encloses_string_with_four_occurences);
    add_test_with_context(suite, enclose_word_in_string, handles_all_null_input);
    add_test_with_context(suite, enclose_word_in_string, handles_null_sentence_rest_non_null);
    add_test_with_context(suite, enclose_word_in_string, properly_encloses_at_end);
    add_test_with_context(suite, enclose_word_in_string, properly_encloses_at_start);
    return suite;
}
