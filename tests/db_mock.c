
#include "db.h"
#include <cgreen/mocks.h>

struct database_s {
    int dummy;
};

database_t *db_open(char *dbpath, bool readonly) {
    mock(dbpath, readonly);
    return new(database_t, 1);
}

void db_close(database_t *db) {
    mock(db);
}

void db_put_dictent(database_t *db, dictentry de) {
    mock(db, de.definition.s, de.dictname.s, de.kanji.s, de.reading.s, de.frequency);
}

void db_get_dictents(const database_t *db, s8 headword, dictentry *dict[static 1]) {
    mock(db, headword.s, dict);
}

void db_put_freq(const database_t *db, freqentry fe) {
    mock(db, fe.frequency, fe.reading.s, fe.word.s);
}

i32 db_check_exists(s8 dbpath) {
    mock(dbpath.s);
    return 1; // TODO: how to mock?
}

void db_remove(s8 dbpath) {
    mock(dbpath.s);
}