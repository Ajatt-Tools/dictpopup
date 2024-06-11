#ifndef DP_MESSAGES_H
#define DP_MESSAGES_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NOTIFICATIONS
    #include "platformdep.h"
#endif

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/**
 * Check whether debug mode is enabled and cache the result.
 */
__attribute__((unused)) static bool debug_mode_enabled(void) {
    static int debug_enabled = -1;
    if (debug_enabled == -1) {
        const char *dbg_env = getenv("DP_DEBUG");
        debug_enabled = dbg_env && (strcmp(dbg_env, "1") == 0);
    }
    return debug_enabled;
}

#define dbg(fmt, ...)                                                                              \
    do {                                                                                           \
        if (debug_mode_enabled()) {                                                                \
            printf("\033[0;32mDEBUG\033[0m: %s(%d): " fmt, __FILE__, __LINE__, ##__VA_ARGS__);     \
            putchar('\n');                                                                         \
        }                                                                                          \
    } while (0)

#ifdef NOTIFICATIONS
    #define msg(fmt, ...)                                                                          \
        do {                                                                                       \
            notify(0, fmt, ##__VA_ARGS__);                                                         \
            printf(fmt, ##__VA_ARGS__);                                                            \
            putchar('\n');                                                                         \
        } while (0)
#else
    #define msg(fmt, ...)                                                                          \
        do {                                                                                       \
            printf(fmt, ##__VA_ARGS__);                                                            \
            putchar('\n');                                                                         \
        } while (0)
#endif

#ifdef NOTIFICATIONS
    #define err(fmt, ...)                                                                          \
        do {                                                                                       \
            notify(1, fmt, ##__VA_ARGS__);                                                         \
            fprintf(stderr, "\033[0;31mERROR\033[0m: %s(%d): ", __FILE__, __LINE__);               \
            fprintf(stderr, fmt, ##__VA_ARGS__);                                                   \
            fputc('\n', stderr);                                                                   \
        } while (0)
#else
    #define err(fmt, ...)                                                                          \
        do {                                                                                       \
            fprintf(stderr, "\033[0;31mERROR\033[0m: %s(%d): ", __FILE__, __LINE__);               \
            fprintf(stderr, fmt, ##__VA_ARGS__);                                                   \
            fputc('\n', stderr);                                                                   \
        } while (0)
#endif

#define _die(dump, fmt, ...)                                                                       \
    do {                                                                                           \
        err(fmt, ##__VA_ARGS__);                                                                   \
        if (dump) {                                                                                \
            abort();                                                                               \
        }                                                                                          \
        exit(1);                                                                                   \
    } while (0)
#define die(fmt, ...) _die(0, fmt, ##__VA_ARGS__)

#define die_on(cond, fmt, ...)                                                                     \
    do {                                                                                           \
        if (unlikely(cond)) {                                                                      \
            die(fmt, ##__VA_ARGS__);                                                               \
        }                                                                                          \
    } while (0)

#endif
