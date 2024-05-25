#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mecab.h>

#include "deinflector.h"
#include "messages.h"
#include "util.h"

typedef enum { UNKNOWN, VERB, ADJ_I } wtype;

s8 *deinfs = NULL;
wtype *wtypes = NULL;

#define IF_STARTSWITH_REPLACE(type, prefix, replacement)                                           \
    do {                                                                                           \
        if (startswith(word, S(prefix))) {                                                         \
            add_replace_prefix(word, S(replacement), lengthof(replacement), type);                 \
        }                                                                                          \
    } while (0)

// TODO: Would be cool if the for loop could expand at compile time, so S(..)
// can be used.	Or just use multiple macros with different arguement length
#define IF_ENDSWITH_REPLACE(type, ending, ...)                                                     \
    do {                                                                                           \
        if (endswith(word, S(ending))) {                                                           \
            for (const char **iterator = (const char *[]){__VA_ARGS__, NULL}; *iterator;           \
                 iterator++) {                                                                     \
                add_replace_suffix(word, fromcstr_((char *)*iterator), lengthof(ending), type);    \
            }                                                                                      \
        }                                                                                          \
    } while (0)

#define IF_ICHIDAN_REPLACE(ending, repl)                                                           \
    do {                                                                                           \
        if ((size_t)word.len > lengthof(ending) && isichidan(cuttail(word, lengthof(ending))) &&   \
            endswith(word, S(ending))) {                                                           \
            add_replace_suffix(word, S(repl), lengthof(ending), VERB);                             \
        }                                                                                          \
    } while (0)

#define IF_ENDSWITH_CONVERT_ATOU(ending)                                                           \
    do {                                                                                           \
        if (endswith(word, S(ending))) {                                                           \
            atou_form(cuttail(word, lengthof(ending)));                                            \
        }                                                                                          \
    } while (0)

#define IF_ENDSWITH_CONVERT_ITOU(ending)                                                           \
    do {                                                                                           \
        if (endswith(word, S(ending))) {                                                           \
            itou_form(cuttail(word, lengthof(ending)));                                            \
        }                                                                                          \
    } while (0)

#define IF_EQUALS_ADD(type, str, wordtoadd)                                                        \
    do {                                                                                           \
        if (s8equals(word, S(str))) {                                                              \
            add_deinflection(s8dup(S(str)), type);                                                 \
        }                                                                                          \
    } while (0)

static void add_deinflection(s8 deinf, wtype type) {
    buf_push(deinfs, deinf);
    buf_push(wtypes, type);
}

// works inplace
void kata2hira(s8 kata_in) {
    u8 *h = kata_in.s;
    size i = 0;

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

/**
 * add_replace_suffix:
 * @word: The word whose ending should be replaced
 * @replacement: The UTF-8 encoded string which the ending should be replaced
 * with
 * @suffix_len: The length of the suffix (in bytes) that should be replaced
 *
 * Replaces the last @suffix_len bytes of @word with @replacement.
 */
static void add_replace_suffix(s8 word, s8 replacement, size suffix_len, wtype type) {
    assert(word.len >= suffix_len);

    s8 cutword = cuttail(word, suffix_len);
    s8 replstr = concat(cutword, replacement);

    add_deinflection(replstr, type);
}

/*
 * Same as add_replace_suffix but with prefix
 */
static void add_replace_prefix(s8 word, s8 replacement, size prefix_len, wtype type) {
    assert(word.len >= prefix_len);

    s8 cutword = cuthead(word, prefix_len);
    s8 replstr = concat(replacement, cutword);

    add_deinflection(replstr, type);
}

// Assumes valid utf-8
static i32 contains_katakana(s8 word) {
    u8 *h = word.s;
    size i = 0;

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

static void check_katakana(s8 word, wtype type) {
    if (contains_katakana(word)) {
        s8 hira = s8dup(word);
        kata2hira(hira);
        add_deinflection(hira, type);
    }
}

static i32 isichidan(s8 word) {
    // TODO: make this more efficient
    return endswith(word, S("い")) || endswith(word, S("き")) || endswith(word, S("ぎ")) ||
           endswith(word, S("じ")) || endswith(word, S("ち")) || endswith(word, S("ぢ")) ||
           endswith(word, S("に")) || endswith(word, S("ひ")) || endswith(word, S("び")) ||
           endswith(word, S("み")) || endswith(word, S("り")) || endswith(word, S("え")) ||
           endswith(word, S("け")) || endswith(word, S("げ")) || endswith(word, S("せ")) ||
           endswith(word, S("ぜ")) || endswith(word, S("て")) || endswith(word, S("で")) ||
           endswith(word, S("ね")) || endswith(word, S("へ")) || endswith(word, S("べ")) ||
           endswith(word, S("め")) || endswith(word, S("れ")) || endswith(word, S("寝")) ||
           endswith(word, S("居")) || endswith(word, S("着"));
}

/*
 * @word: The word to be converted
 * @len_ending: The length of the ending to be disregarded
 *
 * Converts a word in あ-form to the う-form.
 */
static void atou_form(s8 word) {
    IF_ICHIDAN_REPLACE("", "る");
    IF_ENDSWITH_REPLACE(VERB, "さ", "す");
    IF_ENDSWITH_REPLACE(VERB, "か", "く");
    IF_ENDSWITH_REPLACE(VERB, "が", "ぐ");
    IF_ENDSWITH_REPLACE(VERB, "ば", "ぶ");
    IF_ENDSWITH_REPLACE(VERB, "た", "つ");
    IF_ENDSWITH_REPLACE(VERB, "ま", "む");
    IF_ENDSWITH_REPLACE(VERB, "わ", "う");
    IF_ENDSWITH_REPLACE(VERB, "な", "ぬ");
    IF_ENDSWITH_REPLACE(VERB, "ら", "る");
}

/*
 * @word: The word to be converted
 * @len_ending: The length of the ending to be disregarded
 *
 * Converts a word in い-form to the う-form.
 */
// TODO: should be called something with verb stem
static void itou_form(s8 word) {
    IF_ENDSWITH_REPLACE(VERB, "し", "する");
    IF_ENDSWITH_REPLACE(VERB, "き", "くる");

    IF_ICHIDAN_REPLACE("", "る");
    IF_ENDSWITH_REPLACE(VERB, "し", "す");
    IF_ENDSWITH_REPLACE(VERB, "き", "く");
    IF_ENDSWITH_REPLACE(VERB, "ぎ", "ぐ");
    IF_ENDSWITH_REPLACE(VERB, "び", "ぶ");
    IF_ENDSWITH_REPLACE(VERB, "ち", "つ");
    IF_ENDSWITH_REPLACE(VERB, "み", "む");
    IF_ENDSWITH_REPLACE(VERB, "い", "う");
    IF_ENDSWITH_REPLACE(VERB, "に", "ぬ");
    IF_ENDSWITH_REPLACE(VERB, "り", "る");
}

static void kanjify(s8 word, wtype type) {
    IF_STARTSWITH_REPLACE(type, "ご", "御");
    IF_STARTSWITH_REPLACE(type, "お", "御");

    IF_ENDSWITH_REPLACE(type, "ない", "無い");
    IF_ENDSWITH_REPLACE(type, "なし", "無し");
    IF_ENDSWITH_REPLACE(type, "つく", "付く");
}

static void check_te(s8 word) {
    IF_EQUALS_ADD(VERB, "きて", "来る");
    IF_ENDSWITH_REPLACE(VERB, "来て", "来る");
    IF_EQUALS_ADD(VERB, "いって", "行く");
    IF_ENDSWITH_REPLACE(VERB, "行って", "行く");

    IF_ENDSWITH_REPLACE(VERB, "して", "する", "す");
    IF_ENDSWITH_REPLACE(VERB, "いて", "く");
    IF_ENDSWITH_REPLACE(VERB, "いで", "ぐ");
    IF_ENDSWITH_REPLACE(VERB, "んで", "む", "ぶ", "ぬ");
    IF_ENDSWITH_REPLACE(VERB, "って", "る", "う", "つ");
    IF_ENDSWITH_REPLACE(VERB, "て", "る");
}

static void check_past(s8 word) {
    /* exceptions */
    IF_ENDSWITH_REPLACE(VERB, "した", "する");
    IF_EQUALS_ADD(VERB, "きた", "来る");
    IF_EQUALS_ADD(VERB, "来た", "来る");
    IF_EQUALS_ADD(VERB, "いった", "行く");
    IF_ENDSWITH_REPLACE(VERB, "行った", "行く");
    /* ----------- */

    IF_ENDSWITH_REPLACE(VERB, "した", "す");
    IF_ENDSWITH_REPLACE(VERB, "いた", "く");
    IF_ENDSWITH_REPLACE(VERB, "いだ", "ぐ");
    IF_ENDSWITH_REPLACE(VERB, "んだ", "む", "ぶ", "ぬ");
    IF_ENDSWITH_REPLACE(VERB, "った", "る", "う", "つ");
    IF_ENDSWITH_REPLACE(VERB, "た", "る");
}

static void check_masu(s8 word) {
    IF_ENDSWITH_CONVERT_ITOU("ます");
    IF_ENDSWITH_CONVERT_ITOU("ません");
    IF_ENDSWITH_CONVERT_ITOU("ませんでした");
    IF_ENDSWITH_CONVERT_ITOU("ましょう");
}

static void check_shimau(s8 word) {
    IF_ENDSWITH_REPLACE(VERB, "しまう", "");
    IF_ENDSWITH_REPLACE(VERB, "ちゃう", "る");
    IF_ENDSWITH_REPLACE(VERB, "いじゃう", "ぐ");
    IF_ENDSWITH_REPLACE(VERB, "いちゃう", "く");
    IF_ENDSWITH_REPLACE(VERB, "しちゃう", "す");
    IF_ENDSWITH_REPLACE(VERB, "んじゃう", "む");
}

static void check_passive_causative(s8 word) {
    IF_ICHIDAN_REPLACE("られる", "る");
    IF_ICHIDAN_REPLACE("させる", "る");
    IF_ENDSWITH_CONVERT_ATOU("れる");
    IF_ENDSWITH_CONVERT_ATOU("せる");
}

static void check_volitional(s8 word) {
    /* -- Exceptions -- */
    IF_ENDSWITH_REPLACE(VERB, "しよう", "する");
    IF_EQUALS_ADD(VERB, "こよう", "来る");
    /* ---------------- */
    IF_ICHIDAN_REPLACE("よう", "る");
    IF_ENDSWITH_REPLACE(VERB, "そう", "す");
    IF_ENDSWITH_REPLACE(VERB, "こう", "く");
    IF_ENDSWITH_REPLACE(VERB, "ごう", "ぐ");
    IF_ENDSWITH_REPLACE(VERB, "ぼう", "ぶ");
    IF_ENDSWITH_REPLACE(VERB, "とう", "つ");
    IF_ENDSWITH_REPLACE(VERB, "もう", "む");
    IF_ENDSWITH_REPLACE(VERB, "おう", "う");
    IF_ENDSWITH_REPLACE(VERB, "のう", "ぬ");
    IF_ENDSWITH_REPLACE(VERB, "ろう", "る");

    IF_ENDSWITH_CONVERT_ITOU("たがる");
}

static void check_negation(s8 word) {
    IF_EQUALS_ADD(VERB, "ない", "有る");
    IF_ENDSWITH_CONVERT_ATOU("ない");
    IF_ENDSWITH_CONVERT_ATOU("ん");
    IF_ENDSWITH_CONVERT_ATOU("ねぇ");
    IF_ENDSWITH_CONVERT_ATOU("ず");
    IF_ENDSWITH_CONVERT_ATOU("ぬ");

    IF_ICHIDAN_REPLACE("まい", "る");
}

static void check_potential(s8 word) {
    /* Exceptions */
    IF_EQUALS_ADD(VERB, "できる", "為る");
    IF_EQUALS_ADD(VERB, "こられる", "来る");
    IF_EQUALS_ADD(VERB, "来られる", "来る");
    /* ---------- */

    /* IF_ICHIDAN_REPLACE("られる", "る"); // covered by passive/causative */
    IF_ENDSWITH_REPLACE(VERB, "せる", "す");
    IF_ENDSWITH_REPLACE(VERB, "ける", "く");
    IF_ENDSWITH_REPLACE(VERB, "げる", "ぐ");
    IF_ENDSWITH_REPLACE(VERB, "べる", "ぶ");
    IF_ENDSWITH_REPLACE(VERB, "てる", "つ");
    IF_ENDSWITH_REPLACE(VERB, "める", "む");
    IF_ENDSWITH_REPLACE(VERB, "れる", "る");
    IF_ENDSWITH_REPLACE(VERB, "ねる", "ぬ");
    IF_ENDSWITH_REPLACE(VERB, "える", "う");
}

static void check_conditional(s8 word) {
    IF_ENDSWITH_REPLACE(VERB, "せば", "す");
    IF_ENDSWITH_REPLACE(VERB, "けば", "く");
    IF_ENDSWITH_REPLACE(VERB, "げば", "ぐ");
    IF_ENDSWITH_REPLACE(VERB, "べば", "ぶ");
    IF_ENDSWITH_REPLACE(VERB, "てば", "つ");
    IF_ENDSWITH_REPLACE(VERB, "めば", "む");
    IF_ENDSWITH_REPLACE(VERB, "えば", "う");
    IF_ENDSWITH_REPLACE(VERB, "ねば", "ぬ");
    IF_ENDSWITH_REPLACE(VERB, "れば", "る");
}

static void check_concurrent(s8 word) {
    IF_ENDSWITH_CONVERT_ITOU("ながら");
}

static void check_imperative(s8 word) {
    IF_ICHIDAN_REPLACE("ろ", "る");
    IF_ENDSWITH_REPLACE(VERB, "れ", "る");
    IF_ENDSWITH_REPLACE(VERB, "せ", "す");
    IF_ENDSWITH_REPLACE(VERB, "け", "く");
    IF_ENDSWITH_REPLACE(VERB, "げ", "ぐ");
    IF_ENDSWITH_REPLACE(VERB, "べ", "ぶ");
    IF_ENDSWITH_REPLACE(VERB, "て", "つ");
    IF_ENDSWITH_REPLACE(VERB, "え", "う");
    IF_ENDSWITH_REPLACE(VERB, "ね", "ぬ");
    IF_ENDSWITH_REPLACE(VERB, "め", "む");
}

static void check_iadjective(s8 word) {
    IF_ENDSWITH_CONVERT_ITOU("たい");

    IF_ENDSWITH_REPLACE(ADJ_I, "よくて", "いい");
    IF_ENDSWITH_REPLACE(ADJ_I, "かった", "い");
    IF_ENDSWITH_REPLACE(ADJ_I, "くて", "い");
    IF_ENDSWITH_REPLACE(ADJ_I, "そう", "い");
    IF_ENDSWITH_REPLACE(ADJ_I, "さ", "い");
    IF_ENDSWITH_REPLACE(ADJ_I, "げ", "い");
    IF_ENDSWITH_REPLACE(ADJ_I, "く", "い");
    IF_ENDSWITH_REPLACE(ADJ_I, "かろう", "い");

    /* negation */
    IF_ENDSWITH_REPLACE(ADJ_I, "くない", "い");

    /* conditional */
    IF_ENDSWITH_REPLACE(ADJ_I, "ければ", "い");
}

static void check_nagara(s8 word) {
    IF_ENDSWITH_CONVERT_ITOU("ながら");
}

static void deinflect_one_iter(s8 word, wtype type) {

    if (type == UNKNOWN || type == VERB) {
        check_shimau(word);
        check_masu(word);
        check_passive_causative(word);
        check_volitional(word);
        check_te(word);
        check_past(word);
        check_potential(word);
        check_conditional(word);
        check_concurrent(word);
        check_imperative(word);
        check_nagara(word);
    }

    if (type == UNKNOWN || type == ADJ_I) {
        check_negation(word);
        check_iadjective(word);
    }

    kanjify(word, type);
}

static wtype classify_word(s8 word) {
    if (endswith(word, S("ない")) || endswith(word, S("たい"))) {
        return ADJ_I;
    }

    return UNKNOWN;
}

s8 *deinflect(s8 word) {
    wtype wt = classify_word(word); // TODO: Extract type from yomichan dict?

    check_katakana(word, wt);
    deinflect_one_iter(word, wt);
    for (size_t i = 0; i < buf_size(deinfs); i++)
        deinflect_one_iter(deinfs[i], wtypes[i]);
    if (wt == UNKNOWN || wt == VERB)
        itou_form(word); // Check for stem form

    buf_free(wtypes);
    s8 *deinfscpy = deinfs;
    deinfs = NULL;
    return deinfscpy;
}

#ifdef DEINFLECTOR_MAIN
int main(int argc, char **argv) {
    die_on(argc < 2, "Usage: ");
    s8 *d = deinflect(fromcstr_(argv[1]));
    for (size_t i = 0; i < buf_size(d); i++) {
        printf("%.*s\n", (int)d[i].len, (char *)d[i].s);
    }
}
#endif
