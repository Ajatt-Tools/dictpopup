#include <cgreen/cgreen.h>

#include "ankiconnectc.h"
#include "db_mock.c"
#include "dictionary_lookup.c"
#include "utils/util.h"

s8 get_windowname(void) {
    return (s8){0};
}

void file_copy_sync(const char *source_path, const char *dest_path) {
    mock(source_path, dest_path);
}

void createdir(s8 dirpath) {
}

bool check_file_exists(const char *fn) {
    return (bool)mock(fn);
}

const char *get_user_data_dir(void) {
    return "/home/user/.local/share";
}

TestSuite *dictpopup_tests(void);

Describe(DictPopup);
BeforeEach(DictPopup) {
}
AfterEach(DictPopup) {
}


Ensure(DictPopup, looks_up_substrings_as_expected) {
    _drop_(frees8) s8 lookup = s8dup(S("世界からはいつの間にか風の音が、虫の鳴き声が消失していた"));
    _drop_(db_close) database_t *db = db_open(S("dummy"), true);

    Dict dict = lookup_first_matching_prefix(&lookup, db);

    assert_that(dict_contains(dict, S("世界")));

    dict_free(dict);
}

Ensure(DictPopup, copies_default_database_if_no_database_exists) {
    _drop_(free) s8 dbdir = buildpath(fromcstr_((char *)get_user_data_dir()), S("dictpopup"));
    _drop_(free) s8 dbfile = buildpath(dbdir, S("data.mdb"));

    always_expect(check_file_exists, will_return(true));
    always_expect(db_check_exists, will_return(false));

    expect(file_copy_sync, when(source_path, is_equal_to_string(DEFAULT_DATABASE_LOCATIONS[0])),
           when(dest_path, is_equal_to_string((char *)dbfile.s)));

    // expect(read_user_settings, )
    // cfg.general.dbDir = (char*)dbdir.s;
}

Ensure(DictPopup, sorts_direct_matches_first) {
    //TODO
}

TestSuite *dictpopup_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, DictPopup, looks_up_substrings_as_expected);
    // add_test_with_context(suite, DictPopup, sorts_direct_matches_first);
    // add_test_with_context(suite, DictPopup, copies_default_database_if_no_database_exists);
    return suite;
}
