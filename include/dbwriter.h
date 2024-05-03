#include "util.h"
#include <stddef.h>

void opendb(char const *dbpath);
void closedb(void);
void addtodb(s8 key, s8 val);
void db_add_int(s8 key, int val);
