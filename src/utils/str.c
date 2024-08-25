
#include "utils/str.h"

#include <string.h>

/* --------------- Start s8 utils -------------- */
i32 u8compare(u8 *restrict a, u8 *restrict b, isize n) {
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
    assert(dst.len >= src.len && src.len >= 0);

    if (src.s) { // Passing null to memcpy is undefined behaviour ...
        memcpy(dst.s, src.s, src.len);
        dst.s += src.len;
        dst.len -= src.len;
    }
    return dst;
}

s8 news8(isize len) {
    assert(len >= 0);
    return (s8){.s = new (u8, len + 1), // Include NULL terminator
                .len = len};
}

s8 s8dup(s8 src) {
#ifdef UNIT_TEST
    if (!src.s)
        return (s8){0};
#endif

    s8 r = news8(src.len);
    s8copy(r, src);
    return r;
}

s8 *s8dup_ptr(s8 src) {
    s8 *ret = new (s8, 1);
    *ret = s8dup(src);
    return ret;
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

s8 cuthead(s8 s, isize off) {
    assert(off >= 0);
    assert(off <= s.len);
    s.s += off;
    s.len -= off;
    return s;
}

s8 takehead(s8 s, isize len) {
    assert(len >= 0);
    assert(len <= s.len);
    s.len = len;
    return s;
}

s8 cuttail(s8 s, isize len) {
    assert(len >= 0);
    assert(len <= s.len);
    s.len -= len;
    return s;
}

s8 taketail(s8 s, isize len) {
    return cuthead(s, s.len - len);
}

bool startswith(s8 s, s8 prefix) {
    return s.len >= prefix.len && s8equals(takehead(s, prefix.len), prefix);
}

bool endswith(s8 s, s8 suffix) {
    return s.len >= suffix.len && s8equals(taketail(s, suffix.len), suffix);
}

s8 unescape(s8 str) {
    isize s = 0, e = 0;
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
    isize len = 0;
    for (s8 *s = strings; !s8equals(*s, S8_STOPPER); s++)
        len += s->len;

    s8 ret = news8(len);
    s8 p = ret;
    for (s8 *s = strings; !s8equals(*s, S8_STOPPER); s++)
        p = s8copy(p, *s);
    return ret;
}

s8 _concat_with_sep(s8 sep, s8 *strings) {
    isize pathlen = 0;
    bool first = true;
    for (s8 *str = strings; !s8equals(*str, S8_STOPPER); str++) {
        if (!first)
            pathlen += sep.len;
        pathlen += str->len;
        first = false;
    }

    s8 retpath = news8(pathlen);
    s8 p = retpath;

    first = true;
    for (s8 *pc = strings; !s8equals(*pc, S8_STOPPER); pc++) {
        if (!first)
            p = s8copy(p, sep);
        p = s8copy(p, *pc);
        first = false;
    }

    return retpath;
}

// TODO: Don't append sep when already there
s8 buildpath_(s8 *pathcomps) {
#ifdef _WIN32
    s8 sep = S("\\");
#else
    s8 sep = S("/");
#endif

    return _concat_with_sep(sep, pathcomps);
}

static bool whitespace(u8 c) {
    switch (c) {
        case '\t':
        case '\n':
        case '\b':
        case '\f':
        case '\r':
        case ' ':
            return true;
        default:
            return false;
    }
}

void strip_trailing_whitespace(s8 *str) {
    u8 *lastchr = str->s + str->len - 1;
    while (str->len > 0 && whitespace(*lastchr)) {
        str->len--;
        lastchr--;
    }
    (str->s)[str->len] = '\0';
}

/*
 * Caller takes ownership of the data
 */
s8 enclose_word_in_s8_with(s8 str, s8 word, s8 prefix, s8 suffix) {
    if (str.len == 0 || word.len == 0 || (prefix.len == 0 && suffix.len == 0))
        return str;

    assert(str.s[str.len] == '\0');
    stringbuilder_s sb = sb_init(str.len + 2 * (prefix.len + suffix.len));
    char *p = (char *)str.s;
    char *prev = p;
    while ((p = strstr(p, (char *)word.s)) != NULL) {
        isize len = p - prev;
        sb_append(&sb, (s8){.s = (u8 *)prev, .len = len});
        sb_append(&sb, prefix);
        sb_append(&sb, word);
        sb_append(&sb, suffix);
        p += word.len;
        prev = p;
    }
    sb_append(&sb, fromcstr_(prev));
    return sb_steals8(sb);
}

void frees8(s8 *z) {
    free(z->s);
}

/* --------------- Start stringbuilder --------------- */

stringbuilder_s sb_init(size_t init_cap) {
    return (stringbuilder_s){.len = 0, .cap = init_cap, .data = xcalloc(1, init_cap)};
}

void sb_append(stringbuilder_s *b, s8 str) {
    assert(b->cap > 0);
    if (b->cap < b->len + str.len) {
        while (b->cap < b->len + str.len) {
            b->cap *= 2;
            assume(b->cap > 0); // Integer overflow check
        }

        b->data = xrealloc(b->data, b->cap);
    }

    for (int i = 0; i < str.len; i++)
        b->data[b->len + i] = str.s[i];
    b->len += str.len;
}

void sb_append_char(stringbuilder_s *sb, char c) {
    sb_append(sb, (s8){.s = (u8 *)&c, .len = 1});
}

/*
 * Returns the corresponding s8 to @sb
 */
s8 sb_gets8(stringbuilder_s sb) {
    return (s8){.s = sb.data, .len = sb.len};
}

/*
 * Frees the string builder and returns the corresponding s8.
 */
s8 sb_steals8(stringbuilder_s sb) {
    s8 ret = {0};
    ret.len = sb.len;
    ret.s = (u8 *)sb_steal_str(&sb);
    return ret;
}

char *sb_steal_str(stringbuilder_s *sb) {
    sb_append_char(sb, '\0');
    char *r = (char *)sb->data;
    *sb = (stringbuilder_s){0};
    return r;
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

void substrremove(char *str, const s8 sub) {
    assert(sub.s[sub.len] == '\0');
    if (!str)
        return;

    if (sub.len > 0) {
        char *p = str;
        while ((p = strstr(p, (char *)sub.s)) != NULL) {
            memmove(p, p + sub.len, strlen(p + sub.len) + 1);
        }
    }
}

// TODO: Cleaner (and non-null terminated) implementation
void nuke_whitespace(s8 z[static 1]) {
    substrremove((char *)z->s, S("\n"));
    substrremove((char *)z->s, S("\t"));
    substrremove((char *)z->s, S(" "));
    substrremove((char *)z->s, S("　"));

    *z = fromcstr_((char *)z->s);
}

void s8_buf_free(s8Buf buf) {
    while (buf_size(buf) > 0)
        free(buf_pop(buf).s);
    buf_free(buf);
}

size_t s8_buf_size(s8Buf buf) {
    return buf_size(buf);
}

void s8_buf_print(s8Buf buf) {
    for (size_t i = 0; i < s8_buf_size(buf); i++) {
        printf("%.*s\n", (int)buf[i].len, (char *)buf[i].s);
    }
}