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
extern u8 const utf8_chr_len_data[];

s8 convert_to_utf8(char *str);

/*
 * Strips last utf8 character
 * Warning: Not NULL-terminated
 */
s8 striputf8chr(s8 s);


#endif //UTF8_H
