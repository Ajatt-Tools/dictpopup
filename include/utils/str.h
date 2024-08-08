#ifndef S8_H
#define S8_H

#include "utils/util.h"
#include <stdbool.h>

typedef struct {
    u8 *s;
    isize len;
} s8;

// This encodes an invalid UTF-8 char to be used as a stopper for variable argument macros
#define S8_STOPPER                                                                                 \
    (s8) {                                                                                         \
        .s = (u8[]){0xF8}, .len = 1                                                                \
    }
#define lengthof(s) (arrlen("" s "") - 1)
#define S(s) (s8){(u8 *)s, arrlen(s) - 1}

/*
 * Allocates a new s8 with length @len
 * The containing string is null-terminated
 */
s8 news8(isize len);

i32 u8compare(u8 *a, u8 *b, isize n);
/*
 * Copies @src into @dst returning the remaining portion of @dst
 */
s8 s8copy(s8 dst, s8 src);

/*
 * Returns a copy of s
 */
s8 s8dup(s8 src);
s8 *s8dup_ptr(s8 src);
/*
 * Turns @z into an s8 string, reusing the pointer.
 */
s8 fromcstr_(char *z);
/*
 * Returns a true value if a is equal to b
 */
i32 s8equals(s8 a, s8 b);

s8 cuthead(s8 s, isize off);
s8 takehead(s8 s, isize len);
s8 cuttail(s8 s, isize len);
s8 taketail(s8 s, isize len);
bool startswith(s8 s, s8 prefix);
bool endswith(s8 s, s8 suffix);

/*
 * Returns s with the last UTF-8 character removed
 * Expects s to have length > 0
 */
s8 s8striputf8chr(s8 s);

/*
 * Turns escaped characters such as the string "\\n" into the character '\n'
 * (inplace)
 *
 * Returns: unescaped string
 */
s8 unescape(s8 str);

void strip_trailing_whitespace(s8 *str);

void _nonnull_ frees8(s8 *z);
DEFINE_DROP_FUNC_PTR(s8, frees8)

void frees8buffer(s8 *buf);
DEFINE_DROP_FUNC(s8 *, frees8buffer)

/*
 * Concatenates all s8 strings passed as argument
 *
 * Returns: A newly allocated s8 containing the concatenated string
 */
#define concat(...) concat_((s8[]){__VA_ARGS__, S8_STOPPER})
s8 _nonnull_ concat_(s8 *strings);

#define buildpath(...) buildpath_((s8[]){__VA_ARGS__, S8_STOPPER})
s8 _nonnull_ buildpath_(s8 *pathcomps);
s8 enclose_word_in_s8_with(s8 str, s8 word, s8 prefix, s8 suffix);

/* --------------------------- string builder -----------------------_ */
typedef struct {
    u8 *data;
    isize len;
    isize cap;
} stringbuilder_s;

stringbuilder_s sb_init(size_t init_cap);
void sb_append(stringbuilder_s *b, s8 str);
void sb_append_char(stringbuilder_s *sb, char c);
char *sb_steal_str(stringbuilder_s *sb);
s8 sb_gets8(stringbuilder_s sb);
s8 sb_steals8(stringbuilder_s sb);
void _nonnull_ sb_set(stringbuilder_s *sb, s8 s);
void _nonnull_ sb_free(stringbuilder_s *sb);
/* ------------------------------------------------------------------ */

void substrremove(char *str, const s8 sub);
void nuke_whitespace(s8 z[static 1]);

#endif // S8_H
