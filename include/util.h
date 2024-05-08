#ifndef DP_UTIL_H
#define DP_UTIL_H

#include "buf.h"    // growable buffer implementation
#include <dirent.h> // DIR
#include <stdio.h>  // FILE
#include <unistd.h> // close()

typedef unsigned char u8;
typedef signed int b32;
typedef signed int i32;
typedef unsigned int u32;
typedef __PTRDIFF_TYPE__ size;
typedef char byte;

#define assert(c)                                                                                  \
    while (!(c))                                                                                   \
    __builtin_unreachable()

#define _drop_(x) __attribute__((__cleanup__(drop_##x)))
#define _nonnull_ __attribute__((nonnull))
#define _nonnull_n_(...) __attribute__((nonnull(__VA_ARGS__)))
#define _printf_(a, b) __attribute__((__format__(printf, a, b)))

#define arrlen(x)                                                                                  \
    (__builtin_choose_expr(!__builtin_types_compatible_p(typeof(x), typeof(&*(x))),                \
                           sizeof(x) / sizeof((x)[0]), (void)0 /* decayed, compile error */))

/**
 * Memory allocation wrapper which abort on failure
 */
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
// clang-format off
#define new(type, num) xcalloc(num, sizeof(type))
// clang-format on

/* ------------------- Start s8 utils ---------------- */
// This encodes an invalid UTF-8 char to be used as a stopper
// for variable argument length macros
#define S8_STOPPER                                                                                 \
    (s8) {                                                                                         \
        .s = (u8[]){0xF8}, .len = 1                                                                \
    }
#define lengthof(s) (arrlen("" s "") - 1)
#define s8(s)                                                                                      \
    { (u8 *)s, arrlen(s) - 1 }
#define S(s) (s8) s8(s)

typedef struct {
    u8 *s;
    size len;
} s8;

/*
 * Allocates a new s8 with length @len
 * The containing string is null-terminated
 */
s8 news8(size len);

void u8copy(u8 *restrict dst, const u8 *restrict src, size n);
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
s8 fromcstr_(char *z);
/*
 * Returns a true value if a is equal to b
 */
i32 s8equals(s8 a, s8 b);

s8 cuthead(s8 s, size off);
s8 takehead(s8 s, size len);
s8 cuttail(s8 s, size len);
s8 taketail(s8 s, size len);
b32 startswith(s8 s, s8 prefix);
b32 endswith(s8 s, s8 suffix);

/*
 * Returns s with the last UTF-8 character removed
 * Expects s to have length > 0
 */
s8 s8striputf8chr(s8 s);

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

/*
 * Turns escaped characters such as the string "\\n" into the character '\n'
 * (inplace)
 *
 * Returns: unescaped string
 */
s8 unescape(s8 str);

void _nonnull_ frees8(s8 *z);
void _nonnull_ frees8buffer(s8 **buf);

/*
 * Concatenates all s8 strings passed as argument
 *
 * Returns: A newly allocated s8 containing the concatenated string
 */
#define concat(...) concat_((s8[]){__VA_ARGS__, S8_STOPPER})
s8 _nonnull_ concat_(s8 *strings);

#define buildpath(...) buildpath_((s8[]){__VA_ARGS__, S8_STOPPER})
s8 _nonnull_ buildpath_(s8 *pathcomps);
/* ------------------- End s8 utils ---------------- */

/* --------------------------- string builder -----------------------_ */
typedef struct {
    u8 *data;
    size len;
    size cap;
} stringbuilder_s;

stringbuilder_s sb_init(size_t init_cap);
void sb_append(stringbuilder_s *b, s8 str);
s8 sb_gets8(stringbuilder_s sb);
void _nonnull_ sb_set(stringbuilder_s *sb, s8 s);
void _nonnull_ sb_free(stringbuilder_s *sb);
/* ------------------------------------------------------------------_ */

/* --------------------- Start dictentry / dictionary ----------------- */
typedef struct {
    s8 dictname;
    s8 kanji;
    s8 reading;
    s8 definition;
    int frequency;
} dictentry;

dictentry dictentry_dup(dictentry de);
void _nonnull_ dictentry_free(dictentry *de);
void dictentry_print(dictentry de);
void _nonnull_ dictionary_add(dictentry **dict, dictentry de);
size dictlen(dictentry *dict);
void _nonnull_ dictionary_free(dictentry **dict);
dictentry dictentry_at_index(dictentry *dict, size index);
/* --------------------- End dictentry ------------------------ */

size_t _printf_(3, 4) snprintf_safe(char *buf, size_t len, const char *fmt, ...);

s8 nuke_whitespace(s8 z);

/*
 * Returns: 0 on success, -1 on failure and sets errno
 */
int _nonnull_ createdir(char *dirpath);

/**
 * __attribute__((cleanup)) functions
 */
#define DEFINE_DROP_FUNC_PTR(type, func)                                                           \
    static inline void drop_##func(type *p) {                                                      \
        func(p);                                                                                   \
    }
#define DEFINE_DROP_FUNC(type, func)                                                               \
    static inline void drop_##func(type *p) {                                                      \
        if (*p)                                                                                    \
            func(*p);                                                                              \
    }
#define DEFINE_DROP_FUNC_VOID(func)                                                                \
    static inline void drop_##func(void *p) {                                                      \
        void **pp = p;                                                                             \
        if (*pp)                                                                                   \
            func(*pp);                                                                             \
    }

static inline void drop_close(int *fd) {
    if (*fd >= 0) {
        close(*fd);
    }
}

DEFINE_DROP_FUNC_VOID(free)
DEFINE_DROP_FUNC(FILE *, fclose)
DEFINE_DROP_FUNC(DIR *, closedir)
DEFINE_DROP_FUNC_PTR(s8, frees8)

#endif
