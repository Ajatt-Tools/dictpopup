#include "deinflector.h"
#include "deinflector/deinflection_rules.h"
#include <mecab.h>
#include <string.h>
#include <utils/messages.h>
#include <utils/str.h>
#include <utils/utf8.h>

#define MIN(A, B) ((A) < (B) ? (A) : (B))

// works inplace
void kata2hira(s8 kata_in) {
    u8 *h = kata_in.s;
    isize i = 0;

    // Assumes valid utf-8 + null-termination
    while (i + 2 < kata_in.len) {
        /* Check that this is within the katakana block from E3 82 A0 to E3 83
         * BF.
         */
        if (h[i] == 0xe3 && (h[i + 1] == 0x82 || h[i + 1] == 0x83)) {
            /* Check that this is within the range of katakana which
               can be converted into hiragana. */
            if ((h[i + 1] == 0x82 && h[i + 2] >= 0xa1) || (h[i + 1] == 0x83 && h[i + 2] <= 0xb6) ||
                (h[i + 1] == 0x83 && (h[i + 2] == 0xbd || h[i + 2] == 0xbe))) {
                /* Byte conversion from katakana to hiragana. */
                if (h[i + 2] >= 0xa0) {
                    h[i + 1] -= 1;
                    h[i + 2] -= 0x20;
                } else {
                    h[i + 1] = h[i + 1] - 2;
                    h[i + 2] += 0x20;
                }
            }
        }
        i += utf8_chr_len(h + i);
    }
}

#define MECAB_CHECK(eval)                                                                          \
    if (!eval) {                                                                                   \
        err("Mecab error: %s\n", mecab_strerror(mecab));                                           \
        mecab_destroy(mecab);                                                                      \
        return (s8){0};                                                                            \
    }

static s8 mecab_extract_reading(const char *feature) {
#define ENTRY_NR 8 // Entry position to be extracted

    size_t len = strlen(feature);
    size_t off = 0;
    for (int ctr = 0; ctr != ENTRY_NR - 1; off++) {
        if (off >= len)
            return (s8){0};

        if (feature[off] == ',')
            ctr++;
    }

    s8 r = {0};
    r.s = (u8 *)(feature + off); // Casting const away!
    for (; feature[off + r.len] != ',' && off + r.len < len; r.len++)
        ;
    return s8dup(r);
}

s8 kanji2hira(s8 input) {
    mecab_t *mecab = mecab_new2("");
    MECAB_CHECK(mecab);

    assert(input.s[input.len] == '\0');
    const mecab_node_t *node = mecab_sparse_tonode(mecab, (char *)input.s);
    MECAB_CHECK(node);

    s8 kana_reading = {0};
    stringbuilder_s sb = sb_init(input.len);
    for (; node; node = node->next) {
        if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE)
            continue;

        kana_reading = mecab_extract_reading(node->feature);
        if (kana_reading.len == 0)
            kana_reading = s8dup((s8){.s = (u8 *)node->surface, .len = node->length});

        kata2hira(kana_reading);
        sb_append(&sb, kana_reading);
        frees8(&kana_reading);
    }

    mecab_destroy(mecab);
    return sb_steals8(sb);
}

s8 *deinfs = NULL;

static void add_deinflection(s8 deinf) {
    buf_push(deinfs, deinf);
}

static void add_replace_suffix(s8 word, s8 new_suffix, isize suffix_len) {
    assert(word.len >= suffix_len);

    s8 word_without_suffix = cuttail(word, suffix_len);
    s8 deinflected = concat(word_without_suffix, new_suffix);
    add_deinflection(deinflected);
}

// TODO: Would be cool if the for loop could expand at compile time, so S(..)
// can be used.	Or just use multiple macros with different arguement length
#define IF_ENDSWITH_REPLACE(ending, ...)                                                           \
    do {                                                                                           \
        if (endswith(word, S(ending))) {                                                           \
            for (const char **iterator = (const char *[]){__VA_ARGS__, NULL}; *iterator;           \
                 iterator++) {                                                                     \
                add_replace_suffix(word, fromcstr_((char *)*iterator), lengthof(ending));          \
            }                                                                                      \
        }                                                                                          \
    } while (0)

static void deinflect_one_iter(s8 word) {
    // TODO: Use character width instead of hardcoded 3. Currently breaks with non-japanese chars
    // 7 characters each 3 bytes (japanese characters)
    for (int len = MIN(word.len, 3 * 7); len > 0; len -= 3) {
        s8 ending = taketail(word, len);
        DeinflectionRule *rule = in_word_set((char *)ending.s, ending.len);

        if (rule) {
            for (int i = 0; i < MAX_OUTPUTS; i++) {
                s8 kana_out = fromcstr_((char *)rule->kana_out[i]);
                if (!kana_out.len)
                    break;
                add_replace_suffix(word, kana_out, ending.len);
            }
        }
    }
}

// Assumes valid utf-8
static i32 contains_katakana(s8 word) {
    u8 *h = word.s;
    isize i = 0;

    // Assumes valid utf-8 + null-termination
    while (i + 2 < word.len) {
        /* Check that this is within the katakana block from E3 82 A0 to E3 83
         * BF.
         */
        if (h[i] == 0xe3 && (h[i + 1] == 0x82 || h[i + 1] == 0x83)) {
            if ((h[i + 1] == 0x82 && h[i + 2] >= 0xa1) || (h[i + 1] == 0x83 && h[i + 2] <= 0xb6) ||
                (h[i + 1] == 0x83 && (h[i + 2] == 0xbd || h[i + 2] == 0xbe))) {
                return 1;
            }
        }
        i += utf8_chr_len(h + i);
    }
    return 0;
}

static void check_katakana(s8 word) {
    if (contains_katakana(word)) {
        s8 hira = s8dup(word);
        kata2hira(hira);
        add_deinflection(hira);
    }
}

s8 *deinflect(s8 word) {
    check_katakana(word);
    deinflect_one_iter(word);
    for (size_t i = 0; i < buf_size(deinfs); i++)
        deinflect_one_iter(deinfs[i]);

    s8 *deinfscpy = deinfs;
    deinfs = NULL;
    return deinfscpy;
}
