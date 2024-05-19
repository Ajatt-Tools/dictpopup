#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

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

Ensure(AnkiConnectC, sends_correct_search_request) {
    _drop_(frees8) s8 received = form_search_req(false, "Japanese", "VocabKanji", "敷衍");
    s8 expected = S(
        "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : \"\\\"deck:Japanese\\\" \\\"VocabKanji:敷衍\\\" -is:suspended\" } }");
    assert_that(received.s, is_equal_to_string((const char *)expected.s));
}

TestSuite *ankiconnect_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, AnkiConnectC, sends_correct_guiBrowse_request);
    add_test_with_context(suite, AnkiConnectC, sends_correct_search_request);
    return suite;
}
