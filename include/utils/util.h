#ifndef DP_UTIL_H
#define DP_UTIL_H

#include "buf.h"    // growable buffer implementation
#include <stdio.h>  // FILE
#include <unistd.h> // close()

typedef unsigned char u8;
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
__attribute__((malloc, returns_nonnull)) _deallocator_(free) void *xcalloc(size_t nmemb,
                                                                           size_t size);
__attribute__((malloc, returns_nonnull)) _deallocator_(free) void *xrealloc(void *ptr, size_t size);
// clang-format off
#define new(type, num) xcalloc(num, sizeof(type))
// clang-format on

size_t _printf_(3, 4) snprintf_safe(char *buf, size_t len, const char *fmt, ...);

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

#endif
