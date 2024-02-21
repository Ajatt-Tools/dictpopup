#include <stddef.h>

void opendb(char const* dbpath);
void closedb(void);
void addtodb(unsigned char* key_str, ptrdiff_t keylen, unsigned char* value_str, ptrdiff_t vallen);
