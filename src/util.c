#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // mkdir

#include "util.h"

u8 const utf8_chr_len_data[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0};

#define likely(x) __builtin_expect(!!(x), 1)
#define expect(x)                                                                                  \
    do {                                                                                           \
        if (!likely(x)) {                                                                          \
            fprintf(stderr, "FATAL: !(%s) at %s:%s:%d\n", #x, __FILE__, __func__, __LINE__);       \
            abort();                                                                               \
        }                                                                                          \
    } while (0)

void *xmalloc(size_t nbytes) {
    void *p = malloc(nbytes);
    if (!p) {
        perror("malloc");
        abort();
    }
    return p;
}

void *xcalloc(size_t num, size_t nbytes) {
    void *p = calloc(num, nbytes);
    if (!p) {
        perror("calloc");
        abort();
    }
    return p;
}

void *xrealloc(void *ptr, size_t nbytes) {
    void *p = realloc(ptr, nbytes);
    if (!p) {
        perror("realloc");
        abort();
    }
    return p;
}
/* --------------- Start arena ----------------- */

/* typedef struct { */
/*     byte *beg; */
/*     byte *end; */
/* } arena; */

/* arena */
/* newarena_(void) */
/* { */
/*     arena a = {0}; */
/*     size cap = 1<<22; */
/*     a.beg = (byte*)xmalloc(cap); */
/*     a.end = a.beg + cap; */
/*     return a; */
/* } */

/* --------------------------------------------- */

/* --------------- Start s8 utils -------------- */
void u8copy(u8 *restrict dst, const u8 *restrict src, size n) {
    assert(n >= 0);
    for (; n; n--)
        *dst++ = *src++;
}

i32 u8compare(u8 *restrict a, u8 *restrict b, size n) {
    for (; n; n--) {
        i32 d = *a++ - *b++;
        if (d)
            return d;
    }
    return 0;
}

/*
 * Copy src into dst returning the remaining portion of dst.
 */
s8 s8copy(s8 dst, s8 src) {
    assert(dst.len >= src.len);

    u8copy(dst.s, src.s, src.len);
    dst.s += src.len;
    dst.len -= src.len;
    return dst;
}

s8 news8(size len) {
    return (s8){.s = new (u8, len + 1), // Include NULL terminator
                .len = len};
}

s8 s8dup(s8 s) {
    s8 r = news8(s.len);
    u8copy(r.s, s.s, s.len);
    return r;
}

s8 fromcstr_(char *z) {
    s8 s = {0};
    s.s = (u8 *)z;
    s.len = z ? strlen(z) : 0;
    return s;
}

i32 s8equals(s8 a, s8 b) {
    return a.len == b.len && !u8compare(a.s, b.s, a.len);
}

s8 cuthead(s8 s, size off) {
    assert(off >= 0);
    assert(off <= s.len);
    s.s += off;
    s.len -= off;
    return s;
}

s8 takehead(s8 s, size len) {
    assert(len >= 0);
    assert(len <= s.len);
    s.len = len;
    return s;
}

s8 cuttail(s8 s, size len) {
    assert(len >= 0);
    assert(len <= s.len);
    s.len -= len;
    return s;
}

s8 taketail(s8 s, size len) {
    return cuthead(s, s.len - len);
}

b32 startswith(s8 s, s8 prefix) {
    return s.len >= prefix.len && s8equals(takehead(s, prefix.len), prefix);
}

b32 endswith(s8 s, s8 suffix) {
    return s.len >= suffix.len && s8equals(taketail(s, suffix.len), suffix);
}

s8 s8striputf8chr(s8 s) {
    // 0x80 = 10000000; 0xC0 = 11000000
    assert(s.len > 0);
    s.len--;
    while (s.len > 0 && (s.s[s.len] & 0x80) != 0x00 && (s.s[s.len] & 0xC0) != 0xC0)
        s.len--;
    return s;
}

s8 unescape(s8 str) {
    size s = 0, e = 0;
    while (e < str.len) {
        if (str.s[e] == '\\' && e + 1 < str.len) {
            switch (str.s[e + 1]) {
                case 'n':
                    str.s[s++] = '\n';
                    e += 2;
                    break;
                case '\\':
                    str.s[s++] = '\\';
                    e += 2;
                    break;
                case '"':
                    str.s[s++] = '"';
                    e += 2;
                    break;
                case 'r':
                    str.s[s++] = '\r';
                    e += 2;
                    break;
                default:
                    str.s[s++] = str.s[e++];
            }
        } else
            str.s[s++] = str.s[e++];
    }
    str.len = s;

    return str;
}

s8 concat_(s8 *strings) {
    size len = 0;
    for (s8 *s = strings; !s8equals(*s, S8_STOPPER); s++)
        len += s->len;

    s8 ret = news8(len);
    s8 p = ret;
    for (s8 *s = strings; !s8equals(*s, S8_STOPPER); s++)
        p = s8copy(p, *s);
    return ret;
}

s8 buildpath_(s8 *pathcomps) {
#ifdef _WIN32
    s8 sep = S("\\");
#else
    s8 sep = S("/");
#endif
    size pathlen = 0;
    bool first = true;
    for (s8 *pc = pathcomps; !s8equals(*pc, S8_STOPPER); pc++) {
        if (!first)
            pathlen += sep.len;
        pathlen += pc->len;

        first = false;
    }

    s8 retpath = news8(pathlen);
    s8 p = retpath;

    first = true;
    for (s8 *pc = pathcomps; !s8equals(*pc, S8_STOPPER); pc++) {
        if (!first)
            p = s8copy(p, sep);
        p = s8copy(p, *pc);

        first = false;
    }

    return retpath;
}

void frees8(s8 *z) {
    free(z->s);
}

void frees8buffer(s8 **buf) {
    while (buf_size(*buf) > 0)
        free(buf_pop(*buf).s);
    buf_free(*buf);
}

/* -------------- Start dictentry / dictionary utils ---------------- */

dictentry dictentry_dup(dictentry de) {
    return (dictentry){.dictname = s8dup(de.dictname),
                       .kanji = s8dup(de.kanji),
                       .reading = s8dup(de.reading),
                       .definition = s8dup(de.definition)};
}

void dictentry_print(dictentry de) {
    printf("dictname: %s\n"
           "kanji: %s\n"
           "reading: %s\n"
           "definition: %s\n",
           de.dictname.s, de.kanji.s, de.reading.s, de.definition.s);
}

void dictionary_add(dictentry **dict, dictentry de) {
    buf_push(*dict, de);
}

size dictlen(dictentry *dict) {
    return buf_size(dict);
}

void dictentry_free(dictentry *de) {
    frees8(&de->dictname);
    frees8(&de->kanji);
    frees8(&de->reading);
    frees8(&de->definition);
}

void dictionary_free(dictentry **dict) {
    while (buf_size(*dict) > 0)
        dictentry_free(&buf_pop(*dict));
    buf_free(*dict);
}

dictentry dictentry_at_index(dictentry *dict, size index) {
    assert(index >= 0 && (size_t)index < buf_size(dict));
    return dict[index];
}
/* -------------- End dictentry / dictionary utils ---------------- */

/* --------------- Start stringbuilder --------------- */

stringbuilder_s sb_init(size_t init_cap) {
    return (stringbuilder_s){.len = 0, .cap = init_cap, .data = xcalloc(1, init_cap)};
}

void sb_append(stringbuilder_s *b, s8 str) {
    assert(b->cap > 0);
    if (b->cap < b->len + str.len) {
        while (b->cap < b->len + str.len) {
            b->cap *= 2;

            if (b->cap < 0) {
                fputs("Integer Overflow in sb_append()", stderr);
                abort();
            }
        }

        b->data = xrealloc(b->data, b->cap);
    }

    for (int i = 0; i < str.len; i++)
        b->data[b->len + i] = str.s[i];
    b->len += str.len;
}

s8 sb_gets8(stringbuilder_s sb) {
    return (s8){.s = sb.data, .len = sb.len};
}

void sb_set(stringbuilder_s *sb, s8 s) {
    sb->len = 0;
    sb_append(sb, s);
}

void sb_free(stringbuilder_s *sb) {
    free(sb->data);
    *sb = (stringbuilder_s){0};
}
/* --------------- End stringbuilder --------------- */

/**
 * Performs safe, bounded string formatting into a buffer. On error or
 * truncation, expect() aborts.
 */
size_t snprintf_safe(char *buf, size_t len, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(buf, len, fmt, args);
    va_end(args);
    expect(needed >= 0 && (size_t)needed < len);
    return (size_t)needed;
}

// TODO: Rewrite these
static void _nonnull_ remove_substr(char *str, s8 sub) {
    char *s = str, *e = str;

    do {
        while (strncmp(e, (char *)sub.s, sub.len) == 0) {
            e += sub.len;
        }
    } while ((*s++ = *e++));
}

s8 nuke_whitespace(s8 z) {
    remove_substr((char *)z.s, S("\n"));
    remove_substr((char *)z.s, S("\t"));
    remove_substr((char *)z.s, S(" "));
    remove_substr((char *)z.s, S("　"));

    return fromcstr_((char *)z.s);
}

int createdir(char *dirpath) {
    // TODO: Recursive implementation
    int status = mkdir(dirpath, S_IRWXU | S_IRWXG | S_IXOTH);
    return (status == 0 || errno == EEXIST) ? 0 : -1;
}
