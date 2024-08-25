#include <cgreen/cgreen.h>

#include "ankiconnectc/ankiconnectc.c"
#include "utils/util.h"
#include <utils/str.h>

#include <cgreen/mocks.h>

TestSuite *ankiconnect_tests(void);

retval_s sendRequest(s8 request, ResponseFunc response_checker) {
    mock(request.s);
    return (retval_s){.ok = true};
}

Describe(AnkiConnectC);
BeforeEach(AnkiConnectC) {
}
AfterEach(AnkiConnectC) {
}

Ensure(AnkiConnectC, sends_correct_guiBrowse_request) {
    char *expected = "{ \"action\": \"guiBrowse\", \"version\": 6, \"params\": { \"query\" : "
                     "\"\\\"deck:Japanese\\\" \\\"VocabKanji:面白い\\\"\" } }";

    expect(sendRequest, when(request.s, is_equal_to_string(expected)));

    ac_gui_search("Japanese", "VocabKanji", "面白い", 0);
}

Ensure(AnkiConnectC, sends_correct_search_request_without_suspended) {
    _drop_(frees8) s8 received = form_search_req(false, true, "Japanese", "VocabKanji", "敷衍");
    const char *expected = "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : "
                           "\"\\\"deck:Japanese\\\" \\\"VocabKanji:敷衍\\\" -is:suspended\" } }";
    assert_that(received.s, is_equal_to_string(expected));
}

Ensure(AnkiConnectC, sends_correct_search_request_without_new) {
    _drop_(frees8) s8 received = form_search_req(true, false, "Japanese", "VocabKanji", "怒涛");
    const char *expected = "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : "
                           "\"\\\"deck:Japanese\\\" \\\"VocabKanji:怒涛\\\" -is:new\" } }";
    assert_that(received.s, is_equal_to_string(expected));
}

Ensure(AnkiConnectC, sends_correct_search_request_without_new_and_suspended) {
    _drop_(frees8) s8 received = form_search_req(false, false, "Japanese", "VocabKanji", "寿命");
    const char *expected =
        "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : "
        "\"\\\"deck:Japanese\\\" \\\"VocabKanji:寿命\\\" -is:suspended -is:new\" } }";
    assert_that(received.s, is_equal_to_string(expected));
}

Ensure(AnkiConnectC, sends_correct_addNote_request) {
    char *fieldNames[] = {"SentKanji", "VocabKanji", "VocabFurigana", "VocabDef", "Notes"};
    char *fieldEntries[] = {
        "白鯨同様、世界中が被害を被っている。騎士団も長く辛酸を味わわされてきた相手だ", "辛酸",
        "辛酸[しんさん]", "つらい思い。苦しみ。", "Re:Zero"};
    char *tags[] = {"someTag", NULL};
    AnkiCard ac = (AnkiCard){.deck = "Japanese",
                             .notetype = "Japanese Sentences",
                             .num_fields = arrlen(fieldNames),
                             .fieldnames = fieldNames,
                             .fieldentries = fieldEntries,
                             .tags = tags};
    const char *expected =
        "{\"action\": \"addNote\",\"version\": 6,\"params\": {\"note\": {\"deckName\": "
        "\"Japanese\",\"modelName\": \"Japanese Sentences\",\"fields\": {\"SentKanji\" : "
        "\"白鯨同様、世界中が被害を被っている。騎士団も長く辛酸を味わわされてきた相手だ\","
        "\"VocabKanji\" : \"辛酸\",\"VocabFurigana\" : \"辛酸[しんさん]\",\"VocabDef\" : "
        "\"つらい思い。苦しみ。\",\"Notes\" : \"Re:Zero\"},\"options\": {\"allowDuplicate\": "
        "true},\"tags\": [\"someTag\"]}}}";

    expect(sendRequest, when(request.s, is_equal_to_string(expected)));

    char *error = NULL;

    ac_addNote(ac, &error);
}

Ensure(AnkiConnectC, json_escapes_ankicard) {
    char *str = "abcBack\\Slash\"A quote\"\b\f\n\r\t";
    AnkiCard ac = {
        .deck = str,
        .notetype = str,
        .num_fields = 2,
        .fieldnames = (char *[]){str, str, NULL},
        .fieldentries = (char *[]){str, str, NULL},
        .tags = (char *[]){str, str, str, NULL},
    };
    _drop_(ankicard_free) AnkiCard ac_je = ankicard_dup_json_esc(ac);

    char *escstr = "abcBack\\\\Slash\\\"A quote\\\"\\b\\f<br>\\r&#9";
    AnkiCard ac_expect = {
        .deck = escstr,
        .notetype = escstr,
        .num_fields = 2,
        .fieldnames = (char *[]){escstr, escstr, NULL},
        .fieldentries = (char *[]){escstr, escstr, NULL},
        .tags = (char *[]){escstr, escstr, escstr, NULL},
    };
    assert_that(ac_je.deck, is_equal_to_string(ac_expect.deck));
    assert_that(ac_je.notetype, is_equal_to_string(ac_expect.notetype));
    assert_that(ac_je.num_fields, is_equal_to(ac_expect.num_fields));
    // TODO: Finish check
}

static retval_s call_callback_with(s8 response, ResponseFunc callback) {
    retval_s ret = {0};
    callback((char *)response.s, 1, response.len, &ret);
    return ret;
}

Ensure(AnkiConnectC, properly_parses_string_array_response) {
    s8 response = S("{\"result\": [\"Basic\", \"Basic (and reversed card)\"], \"error\": null}");

    retval_s ret = call_callback_with(response, parse_result_array);

    char **parsed = ret.data.strv;
    assert_that(ret.ok, is_true);
    assert_that(parsed[0], is_equal_to_string("Basic"));
    assert_that(parsed[1], is_equal_to_string("Basic (and reversed card)"));

    g_strfreev(ret.data.strv);
}

Ensure(AnkiConnectC, properly_parses_error_1) {
    s8 response = S("{\"result\": null, \"error\": \"unsupported action\"}");

    retval_s ret = call_callback_with(response, check_error_callback);

    assert_that(ret.ok, is_false);
    assert_that(ret.data.string, is_equal_to_string("unsupported action"));

    ac_retval_free(ret);
}

Ensure(AnkiConnectC, properly_parses_error_2) {
    s8 response = S("{\"result\": null, \"error\": \"guiBrowse() got an unexpected keyword "
                    "argument 'foobar'\"}");

    retval_s ret = call_callback_with(response, check_error_callback);

    assert_that(ret.ok, is_false);
    assert_that(ret.data.string,
                is_equal_to_string("guiBrowse() got an unexpected keyword argument 'foobar'"));

    ac_retval_free(ret);
}

Ensure(AnkiConnectC, properly_parses_error_3) {
    s8 response =
        S("{ \"canAdd\": false, \"error\": \"cannot create note because it is a duplicate\" }");

    retval_s ret = call_callback_with(response, check_error_callback);

    assert_that(ret.ok, is_false);
    assert_that(ret.data.string,
                is_equal_to_string("cannot create note because it is a duplicate"));

    ac_retval_free(ret);
}

Ensure(AnkiConnectC, properly_recognizes_existance_of_results_1) {
    s8 response =
        S("{ \"result\": [1494723142483, 1494703460437, 1494703479525], \"error\": null }");

    retval_s ret = call_callback_with(response, has_results_callback);

    assert_that(ret.ok, is_true);
    assert_that(ret.data.boolean, is_true);

    ac_retval_free(ret);
}

Ensure(AnkiConnectC, properly_recognizes_nonexistance_of_results) {
    s8 response = S("{ \"result\": [], \"error\": null }");

    retval_s ret = call_callback_with(response, has_results_callback);

    assert_that(ret.ok, is_true);
    assert_that(ret.data.boolean, is_false);

    ac_retval_free(ret);
}

Ensure(AnkiConnectC, properly_parses_) {
    s8 response = S("{ \"result\": [], \"error\": null }");

    retval_s ret = call_callback_with(response, has_results_callback);

    assert_that(ret.ok, is_true);
    assert_that(ret.data.boolean, is_false);

    ac_retval_free(ret);
}

TestSuite *ankiconnect_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, AnkiConnectC, sends_correct_guiBrowse_request);
    add_test_with_context(suite, AnkiConnectC, sends_correct_search_request_without_suspended);
    add_test_with_context(suite, AnkiConnectC, sends_correct_search_request_without_new);
    add_test_with_context(suite, AnkiConnectC,
                          sends_correct_search_request_without_new_and_suspended);
    add_test_with_context(suite, AnkiConnectC, sends_correct_addNote_request);
    add_test_with_context(suite, AnkiConnectC, json_escapes_ankicard);
    add_test_with_context(suite, AnkiConnectC, properly_parses_string_array_response);
    add_test_with_context(suite, AnkiConnectC, properly_parses_error_1);
    add_test_with_context(suite, AnkiConnectC, properly_parses_error_2);
    add_test_with_context(suite, AnkiConnectC, properly_parses_error_3);
    add_test_with_context(suite, AnkiConnectC, properly_recognizes_existance_of_results_1);
    add_test_with_context(suite, AnkiConnectC, properly_recognizes_nonexistance_of_results);
    return suite;
}
