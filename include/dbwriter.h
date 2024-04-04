#include <stddef.h>
#include "util.h"

void opendb(char const* dbpath);
void closedb(void);
void addtodb(s8 key, s8 val);
void addtodb3(s8 key, int val);
