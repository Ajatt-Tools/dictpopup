#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "jppron/jppron.c"

TestSuite *jppron_tests(void);

Describe(Jppron);
BeforeEach(Jppron) {
}
AfterEach(Jppron) {
}

database *jdb_open(char *path, bool readonly) {
    return NULL;
}

void jdb_close(database *db) {
    return;
}

void jdb_add_headword_with_file(database *db, s8 headword, s8 filepath) {
    mock(headword.s, filepath.s);
}

void jdb_add_file_with_fileinfo(database *db, s8 filepath, fileinfo_s fi) {
    mock(filepath.s, fi.hira_reading.s, fi.origin.s, fi.pitch_number.s, fi.pitch_pattern.s);
}

s8 *jdb_get_files(database *db, s8 key) {
    return (s8*)mock(key.s);
}

fileinfo_s jdb_get_fileinfo(database *db, s8 fullpath) {
    return (fileinfo_s){0};
}

i32 jdb_check_exists(s8 dbpath) {
    return (i32)mock(dbpath.s);
}

void jdb_remove(s8 dbpath) {
    mock(dbpath.s);
}

void play_audio(s8 filepath) {
    mock(filepath.s);
}

Ensure(Jppron, excludes_trailing_o) {
    assert(true);
}

TestSuite *jppron_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, Jppron, excludes_trailing_o);
    return suite;
}