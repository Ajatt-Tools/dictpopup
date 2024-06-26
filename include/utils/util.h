#ifndef DP_UTIL_H
#define DP_UTIL_H

#include "buf.h"    // growable buffer implementation
#include <stdio.h>  // FILE
#include <unistd.h> // close()

typedef unsigned char u8;
typedef signed int b32;
typedef signed int i32;
typedef unsigned int u32;
typedef __PTRDIFF_TYPE__ isize;

#define assert(c)                                                                                  \
    while (!(c))                                                                                   \
    __builtin_unreachable()

#define _drop_(x) __attribute__((__cleanup__(drop_##x)))
#define _nonnull_ __attribute__((nonnull))
#define _nonnull_n_(...) __attribute__((nonnull(__VA_ARGS__)))
#if defined(__clang__)
    #define _deallocator_(x)
#else
    #define _deallocator_(x) __attribute__((malloc(x)))
#endif
#define _printf_(a, b) __attribute__((__format__(printf, a, b)))

#define arrlen(x)                                                                                  \
    (__builtin_choose_expr(!__builtin_types_compatible_p(typeof(x), typeof(&*(x))),                \
                           sizeof(x) / sizeof((x)[0]), (void)0 /* decayed, compile error */))

#define assume(x)                                                                                  \
    do {                                                                                           \
        if (!__builtin_expect(!!(x), 1)) {                                                         \
            fprintf(stderr, "FATAL: !(%s) at %s:%s:%d\n", #x, __FILE__, __func__, __LINE__);       \
            abort();                                                                               \
        }                                                                                          \
    } while (0)

/**
 * Memory allocation wrapper which abort on failure
 */
__attribute__((malloc, returns_nonnull))
_deallocator_(free) void *xcalloc(size_t nmemb, size_t size);
__attribute__((malloc, returns_nonnull)) _deallocator_(free) void *xrealloc(void *ptr, size_t size);
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
    isize len;
} s8;

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
b32 startswith(s8 s, s8 prefix);
b32 endswith(s8 s, s8 suffix);

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
void frees8buffer(s8 *buf);

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
/* ------------------- End s8 utils ---------------- */

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
/* ------------------------------------------------------------------_ */

/* --------------------- Start dictentry / dictionary ----------------- */
typedef struct {
    s8 dictname;
    s8 kanji;
    s8 reading;
    s8 definition;
    u32 frequency;
} dictentry;

dictentry dictentry_dup(dictentry de);
void _nonnull_ dictentry_free(dictentry de);
void dictentry_print(dictentry de);
void _nonnull_ dictionary_add(dictentry **dict, dictentry de);
isize dictlen(dictentry *dict);
void _nonnull_ dictionary_free(dictentry **dict);
dictentry dictentry_at_index(dictentry *dict, isize index);
dictentry *pointer_to_entry_at(dictentry *dict, isize index);
/* --------------------- End dictentry ------------------------ */

/* --------------------- Start freqentry ----------------- */
typedef struct {
    s8 word;
    s8 reading;
    u32 frequency;
} freqentry;

freqentry freqentry_dup(freqentry fe);
void freqentry_free(freqentry *fe);
/* --------------------- End freqentry ------------------------ */

size_t _printf_(3, 4) snprintf_safe(char *buf, size_t len, const char *fmt, ...);

void substrremove(char *str, const s8 sub);


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
DEFINE_DROP_FUNC_PTR(s8, frees8)
DEFINE_DROP_FUNC(s8 *, frees8buffer)

#endif
