#include <cgreen/cgreen.h>

TestSuite *deinflector_tests(void);
TestSuite *ankiconnect_tests(void);
TestSuite *yomichan_parser_tests(void);

int main(int argc, char **argv) {
    TestSuite *suite = create_test_suite();
    add_suite(suite, deinflector_tests());
    add_suite(suite, ankiconnect_tests());
    add_suite(suite, yomichan_parser_tests());

    if (argc > 1) {
        return run_single_test(suite, argv[1], create_text_reporter());
    }

    // This is to keep the leak checker quiet.
    int ret = run_test_suite(suite, create_text_reporter());
    destroy_test_suite(suite);
    return ret;
}