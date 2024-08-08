
#include "db.h"
#include "utils/str.h"

#include <cgreen/mocks.h>

struct database_s {
    int dummy;
};

database_t *db_open(const char *dbpath, bool readonly) {
    return new(database_t, 1);
}

void db_close(database_t *db) {
    free(db);
}

void db_put_dictent(database_t *db, dictentry de) {
    mock(db, de.definition.s, de.dictname.s, de.kanji.s, de.reading.s, de.frequency);
}

void _nonnull_ db_append_lookup(const database_t *db, s8 headword, Dict dict[static 1],
                                bool is_deinflection) {
    if (s8equals(headword, S("世界"))) {
        dictionary_add(dict, (dictentry){ .kanji = S("世界") });
    }
    // mock(db, headword.s, dict);
}

void db_put_freq(const database_t *db, freqentry fe) {
    mock(db, fe.frequency, fe.reading.s, fe.word.s);
}

bool db_check_exists(s8 dbpath) {
    return (bool)mock(dbpath.s);
}

void db_remove(s8 dbpath) {
    mock(dbpath.s);
}