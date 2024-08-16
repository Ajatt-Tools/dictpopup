#include <cgreen/cgreen.h>

#include "ankiconnectc/ankiconnectc.c"
#include "utils/util.h"

#include <cgreen/mocks.h>

TestSuite *ankiconnect_tests(void);

retval_s sendRequest(s8 request, ResponseFunc response_checker) {
    mock(request.s);
    return (retval_s){.ok=true};
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
    const char* expected = "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : "
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
    const char *expected = "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : "
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

TestSuite *ankiconnect_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, AnkiConnectC, sends_correct_guiBrowse_request);
    add_test_with_context(suite, AnkiConnectC, sends_correct_search_request_without_suspended);
    add_test_with_context(suite, AnkiConnectC, sends_correct_search_request_without_new);
    add_test_with_context(suite, AnkiConnectC, sends_correct_search_request_without_new_and_suspended);
    add_test_with_context(suite, AnkiConnectC, sends_correct_addNote_request);
    add_test_with_context(suite, AnkiConnectC, json_escapes_ankicard);
    return suite;
}
