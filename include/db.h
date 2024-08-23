#ifndef DP_DB_H
#define DP_DB_H
#include "objects/dict.h"
#include "objects/freqentry.h"
#include "utils/util.h"
#include <stdbool.h>

// An opaque struct
typedef struct database_s database_t;

// Should not be freed! Value gets cached
s8 db_get_dbpath(void);

database_t *db_open(bool readonly);
void db_close(database_t *db);

void _nonnull_ db_put_dictent(database_t *db, Dictentry de);
void _nonnull_ db_put_freq(database_t *db, freqentry fe);
void _nonnull_ db_put_dictname(database_t *db, s8 dictname);

void _nonnull_ db_append_lookup(const database_t *db, s8 headword, Dict dict[static 1],
                                bool is_deinflection);
s8Buf db_get_dictnames(database_t *db);

bool db_check_exists(s8 dbpath);
void db_remove(s8 dbpath);

DEFINE_DROP_FUNC(database_t *, db_close)

#endif
