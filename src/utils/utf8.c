#include <glib.h>

#include "messages.h"
#include "utils/util.h"
#include "utils/utf8.h"

const u8 utf8_chr_len_data[] = {
    /* 0XXX */ 1, 1, 1, 1, 1, 1, 1, 1,
    /* 10XX */ 1, 1, 1, 1, /* invalid */
    /* 110X */ 2, 2,
    /* 1110 */ 3,
    /* 1111 */ 4, /* maybe, but also could be invalid */
};

s8 convert_to_utf8(char *str) {
    g_autoptr(GError) error = NULL;
    s8 ret = fromcstr_(g_locale_to_utf8(str, -1, NULL, NULL, &error));
    die_on(error, "Converting to UTF-8: %s", error->message);
    return ret;
}

s8 striputf8chr(s8 s) {
    // 0x80 = 10000000; 0xC0 = 11000000
    assert(s.len > 0);
    s.len--;
    while (s.len > 0 && (s.s[s.len] & 0x80) != 0x00 && (s.s[s.len] & 0xC0) != 0xC0)
        s.len--;
    return s;
}