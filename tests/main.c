#include <cgreen/cgreen.h>

TestSuite *deinflector_tests(void);
TestSuite *ankiconnect_tests(void);

int main(int argc, char **argv) {
    TestSuite *suite = create_test_suite();
    add_suite(suite, deinflector_tests());
    add_suite(suite, ankiconnect_tests());

    // This is to prevent the leak checker from failing.
    int ret = run_test_suite(suite, create_text_reporter());
    destroy_test_suite(suite);
    return ret;
}