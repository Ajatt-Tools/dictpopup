#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "jppron/jppron.c"

TestSuite *jppron_tests(void);

Describe(Jppron);
BeforeEach(Jppron) {
}
AfterEach(Jppron) {
}

PronDatabase *jppron_open_db(bool readonly) {
    return NULL;
}

void jppron_close_db(PronDatabase *db) {
    return;
}

void jdb_add_headword_with_file(PronDatabase *db, s8 headword, s8 filepath) {
    mock(headword.s, filepath.s);
}

void jdb_add_file_with_fileinfo(PronDatabase *db, s8 filepath, FileInfo fi) {
    mock(filepath.s, fi.hira_reading.s, fi.origin.s, fi.pitch_number.s, fi.pitch_pattern.s);
}

s8 *jdb_get_files(PronDatabase *db, s8 key) {
    return (s8*)mock(key.s);
}

FileInfo jdb_get_fileinfo(PronDatabase *db, s8 fullpath) {
    return (FileInfo){0};
}

i32 jdb_check_exists(s8 dbpath) {
    return (i32)mock(dbpath.s);
}

void jdb_remove(s8 dbpath) {
    mock(dbpath.s);
}

TestSuite *jppron_tests(void) {
    TestSuite *suite = create_test_suite();
    return suite;
}