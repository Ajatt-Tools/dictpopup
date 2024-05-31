#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "ajt_audio_index_parser.c"

TestSuite *ajt_audio_index_parser_tests(void);

Describe(AudioIndexParser);
BeforeEach(AudioIndexParser) {
}
AfterEach(AudioIndexParser) {
}

struct headword_filepath {
    s8 headword;
    s8 filepath;
};

static void foreach_headword(void *userdata, s8 headword, s8 fullpath) {
    mock(headword.s, fullpath.s);
}

static void foreach_file(void *userdata, s8 fullpath, fileinfo fi) {
    mock(fullpath.s, fi.origin.s, fi.hira_reading.s, fi.pitch_number.s, fi.pitch_pattern.s);
}

Ensure(AudioIndexParser, correctly_parses_nhk2016_entry) {
    s8 curdir = S("../tests/files/testentries/audio_index_1");
    char *fn = "../tests/files/testentries/audio_index_1";

    expect(foreach_headword,
        when(headword.s, is_equal_to_string("静か")),
        when(fullpath.s, is_equal_to_string("../tests/files/testentries/audio_index_1/media/20171018162916.ogg"))
        );
    expect(foreach_file,
        when(fullpath.s, is_equal_to_string("../tests/files/testentries/audio_index_1/media/20171018162916.ogg")),
        when(fi.origin.s, is_equal_to_string("NHK日本語発音アクセント新辞典")),
        when(fi.hira_reading.s, is_equal_to_string("しずか")),
        when(fi.pitch_number.s, is_equal_to_string("1")),
        when(fi.pitch_pattern.s, is_equal_to_string("シ＼ズカ"))
        );
    parse_audio_index_from_file(curdir, fn, &foreach_headword, NULL, &foreach_file, NULL);
}

TestSuite *ajt_audio_index_parser_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, AudioIndexParser, correctly_parses_nhk2016_entry);
    return suite;
}