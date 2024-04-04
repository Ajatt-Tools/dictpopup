#include <stdbool.h>
#include <lmdb.h>
#include "util.h"

typedef struct {
  MDB_env *env;
  MDB_dbi dbi1;
  MDB_dbi dbi2;
  MDB_txn *txn;
  bool readonly;
} database;

database opendb(const char* path, bool readonly);
void closedb(database);
/*
 * Add to database, allowing duplicates if they are added directly after another
 */
void addtodb1(database db, s8 key, s8 val);
void addtodb2(database db, s8 key, s8 val);

s8* getfiles(database db, s8 key);
s8 getfromdb2(database db, s8 key);
