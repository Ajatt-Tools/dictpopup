#ifndef DATABASE_H
#define DATABASE_H

#include "ajt_audio_index_parser.h"
#include "util.h"
#include <stdbool.h>

typedef struct database_s database;

database *jdb_open(char *path, bool readonly);
void jdb_close(database *);
/*
 * Add to database, allowing duplicates if they are added directly after another
 */
void jdb_add_headword_with_file(database *db, s8 headword, s8 filepath);
void jdb_add_file_with_fileinfo(database *db, s8 filepath, fileinfo fi);

s8 *jdb_get_files(database *db, s8 key);
fileinfo jdb_get_fileinfo(database *db, s8 fullpath);

i32 jdb_check_exists(s8 dbpath);

DEFINE_DROP_FUNC(database *, jdb_close)

#endif // DATABASE_H