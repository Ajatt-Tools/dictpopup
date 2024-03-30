#ifndef UTIL_H
#define UTIL_H

#include "buf.h" // Growable buffer implementation

#ifndef __PTRDIFF_TYPE__  // not GCC-like?
#  include <stddef.h>
#  define __PTRDIFF_TYPE__         ptrdiff_t
#  define __builtin_unreachable()  *(volatile int *)0 = 0
#endif

typedef unsigned char    u8;
typedef   signed int     b32;
typedef   signed int     i32;
typedef unsigned int     u32;
typedef __PTRDIFF_TYPE__ size;
typedef          char    byte;

#define assert(c)     while (!(c)) __builtin_unreachable()

/*
 * memory allocation wrapper which abort on failure
 */ 
void* xmalloc(size_t size);
void* xcalloc(size_t nmemb, size_t size);
void* xrealloc(void* ptr, size_t size);
#define new(type, num) xcalloc(num, sizeof(type))

/* ------------------- Start s8 utils ---------------- */
#define countof(a)    (size)(sizeof(a) / sizeof(*(a)))
#define lengthof(s)   (countof("" s "") - 1)
#define s8(s)         {(u8 *)s, countof(s)-1}
#define S(s)          (s8)s8(s)

typedef struct {
    u8* s;
    size len;
} s8;

/*
 * Allocates a new s8 with length @len
 * The containing string is null-terminated
 */
s8 news8(size len);

void u8copy(u8 *dst, u8 *src, size n);
i32 u8compare(u8 *a, u8 *b, size n);
/*
 * Copies @src into @dst returning the remaining portion of @dst
 */
s8 s8copy(s8 dst, s8 src);

/*
 * Returns a copy of s
 */
s8 s8dup(s8 s);
/*
 * Turns @z into an s8 string, reusing the pointer.
 */
s8 fromcstr_(char* z);
/*
 * Returns a true value if a is equal to b
 */
i32 s8equals(s8 a, s8 b);

s8 cuthead(s8 s, size off);
s8 takehead(s8 s, size len);
s8 cuttail(s8 s, size len);
s8 taketail(s8 s, size len);
b32 s8endswith(s8 s, s8 suffix);

/*
 * Returns s with the last UTF-8 character removed
 * Expects s to have length > 0
 */
s8 s8striputf8chr(s8 s);

/*
 * Returns u8 pointer pointing to the start of the char after the first utf-8 character
 */
#define skip_utf8_char(p) (u8*)((p) + utf8_skip[*(const u8 *)(p)])
extern const u8* const utf8_skip;

/*
 * Turns escaped characters such as the string "\\n" into the character '\n' (inplace)
 *
 * Returns: unescaped string
 */
s8 unescape(s8 str);

void frees8(s8 z[static 1]);
void frees8buffer(s8* buf[static 1]);

/*
 * Concatenates all s8 strings passed as argument
 *
 * Returns: A newly allocated s8 containing the concatenated string
 */
#define concat(...) \
    concat_((s8[]){ __VA_ARGS__, S("CONCAT_STOPPER")}, S("CONCAT_STOPPER")) // Might be better to use a stopper that consists of a single invalid char
s8 concat_(s8 strings[static 1], const s8 stopper);

#define buildpath(...) \
    buildpath_((s8[]){ __VA_ARGS__, S("BUILD_PATH_STOPPER")}, S("BUILD_PATH_STOPPER"))
s8 buildpath_(s8 pathcomps[static 1], const s8 stopper);
/* ------------------- End s8 utils ---------------- */

/* --------------------------- string builder -----------------------_ */
typedef struct {
	u8 *data;
    	size len;
    	size cap;
} stringbuilder_s;

stringbuilder_s sb_init(size_t init_cap);
void sb_append(stringbuilder_s* b, s8 str);
s8 sb_gets8(stringbuilder_s sb);
void sb_free(stringbuilder_s* sb);
/* ------------------------------------------------------------------_ */



/* --------------------- Start dictentry / dictionary ----------------- */
typedef struct {
  s8 dictname;
  s8 kanji;
  s8 reading;
  s8 definition;
} dictentry;

dictentry dictentry_dup(dictentry de);
/* void dictentry_print(dictentry de); */
void dictionary_add(dictentry* dict[static 1], dictentry de);
size dictlen(dictentry* dict);
void dictionary_free(dictentry* dict[static 1]);
dictentry dictentry_at_index(dictentry* dict, size index);
/* --------------------- End dictentry ------------------------ */

s8 nuke_whitespace(s8 z);

int printf_cmd_async(char const *fmt, ...);

#endif