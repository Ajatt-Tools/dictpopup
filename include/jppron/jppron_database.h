#ifndef DATABASE_H
#define DATABASE_H

#include "ajt_audio_index_parser.h"
#include "utils/util.h"
#include <stdbool.h>
#include <utils/str.h>
#include "jppron/jppron_database.h"

typedef struct _PronDatabase PronDatabase;

PronDatabase *jppron_open_db(bool readonly);
void jppron_close_db(PronDatabase *);
/*
 * Add to database, allowing duplicates if they are added directly after another
 */
void jdb_add_headword_with_file(PronDatabase *db, s8 headword, s8 filepath);
void jdb_add_file_with_fileinfo(PronDatabase *db, s8 filepath, FileInfo fi);

_deallocator_(s8_buf_free) s8Buf jdb_get_files(PronDatabase *db, s8 key);
FileInfo jdb_get_fileinfo(PronDatabase *db, s8 fullpath);

i32 jdb_check_exists(s8 dbpath);
void jdb_remove(s8 dbpath);

DEFINE_DROP_FUNC(PronDatabase *, jppron_close_db)

#endif // DATABASE_H