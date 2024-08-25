#ifndef UTF8_H
#define UTF8_H

/*
 * Returns u8 pointer pointing to the start of the char after the first utf-8
 * character
 */
#define skip_utf8_char(p) ((p) + utf8_chr_len(p))

/*
 * Returns the length in bytes of the utf8 encoded char pointed to by @p
 */
#define utf8_chr_len(p) utf8_chr_len_data[(p)[0] >> 4]
#include "str.h"
#include "util.h"
extern u8 const utf8_chr_len_data[];

s8 convertToUtf8(char *str);

/*
 * Strips last utf8 character
 * Warning: Not NULL-terminated
 */
void _nonnull_ stripUtf8Char(s8 s[static 1]);

#endif // UTF8_H
