#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/util.h"

void *xcalloc(size_t num, size_t nbytes) {
    void *p = calloc(num, nbytes);
    assume(p);
    return p;
}

void *xrealloc(void *ptr, size_t nbytes) {
    void *p = realloc(ptr, nbytes);
    assume(p);
    return p;
}

/**
 * Performs safe, bounded string formatting into a buffer. On error or
 * truncation, assume() aborts.
 */
size_t snprintf_safe(char *buf, size_t len, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(buf, len, fmt, args);
    va_end(args);
    assume(needed >= 0 && (size_t)needed < len);
    return (size_t)needed;
}
