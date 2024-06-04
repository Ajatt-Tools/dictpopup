#include <cgreen/cgreen.h>

#include "ankiconnectc.c"
#include "util.h"

TestSuite *ankiconnect_tests(void);

Describe(AnkiConnectC);
BeforeEach(AnkiConnectC) {
}
AfterEach(AnkiConnectC) {
}

Ensure(AnkiConnectC, sends_correct_guiBrowse_request) {
    _drop_(frees8) s8 received = form_gui_search_req("Japanese", "VocabKanji", "面白い");
    s8 expected = S(
        "{ \"action\": \"guiBrowse\", \"version\": 6, \"params\": { \"query\" : \"\\\"deck:Japanese\\\" \\\"VocabKanji:面白い\\\"\" } }");
    assert_that(received.s, is_equal_to_string((const char *)expected.s));
}

Ensure(AnkiConnectC, sends_correct_search_request_without_suspended) {
    _drop_(frees8) s8 received = form_search_req(false, true, "Japanese", "VocabKanji", "敷衍");
    s8 expected = S(
        "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : \"\\\"deck:Japanese\\\" \\\"VocabKanji:敷衍\\\" -is:suspended\" } }");
    assert_that(received.s, is_equal_to_string((const char *)expected.s));
}

Ensure(AnkiConnectC, sends_correct_search_request_without_new) {
    _drop_(frees8) s8 received = form_search_req(true, false, "Japanese", "VocabKanji", "敷衍");
    s8 expected = S(
        "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : \"\\\"deck:Japanese\\\" \\\"VocabKanji:敷衍\\\" -is:new\" } }");
    assert_that(received.s, is_equal_to_string((const char *)expected.s));
}

Ensure(AnkiConnectC, json_escapes_ankicard) {
    char *str = "abcBack\\Slash\"A quote\"\b\f\n\r\t";
    ankicard ac = {
        .deck = str,
        .notetype = str,
        .num_fields = 2,
        .fieldnames = (char*[]) { str, str, NULL },
        .fieldentries = (char*[]) { str, str, NULL },
        .tags = (char*[]) { str, str, str, NULL },
    };
    _drop_(ankicard_free) ankicard ac_je = ankicard_dup_json_esc(ac);

    char *escstr = "abcBack\\\\Slash\\\"A quote\\\"\\b\\f<br>\\r&#9";
    ankicard ac_expect = {
        .deck = escstr,
        .notetype = escstr,
        .num_fields = 2,
        .fieldnames = (char*[]) { escstr, escstr, NULL },
        .fieldentries = (char*[]) { escstr, escstr, NULL },
        .tags = (char*[]) { escstr, escstr, escstr, NULL },
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
    add_test_with_context(suite, AnkiConnectC, json_escapes_ankicard);
    return suite;
}
