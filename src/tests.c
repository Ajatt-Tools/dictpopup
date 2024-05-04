#include <stdio.h>

#include "deinflector.h"
#include "util.h"

#define passed(cond) ((cond) ? "passed" : "not passed")

static int test_kana_conversion(void) {
    const s8 all_kata = S("アイウエオカキクケコサシスセソタチツテトナニヌネノハヒ"
                          "フヘホマミムメモヤユヨラリルレロワヲン"
                          "ガギグゲゴザジズゼゾダヂヅデド"
                          "バビブベボ"
                          "パピプペポ");
    const s8 all_hira = S("あいうえおかきくけこさしすせそたちつてとなにぬねのはひ"
                          "ふへほまみむめもやゆよらりるれろわをん"
                          "がぎぐげござじずぜぞだぢづでど"
                          "ばびぶべぼ"
                          "ぱぴぷぺぽ");
    _drop_(frees8) s8 converted_hira = s8dup(all_kata);
    kata2hira(converted_hira);

    return s8equals(converted_hira, all_hira);
}

#define DEINFLECT(expected_deinf, ...)                                                             \
    do {                                                                                           \
        char **words = (char *[]){__VA_ARGS__, NULL};                                              \
        for (char **wordptr = words; *wordptr != NULL; wordptr++) {                                \
            s8 *deinfs = deinflect(fromcstr_(*wordptr));                                           \
            i32 contains = 0;                                                                      \
            for (size_t i = 0; i < buf_size(deinfs); i++) {                                        \
                if (s8equals(deinfs[i], S(expected_deinf))) {                                      \
                    contains = 1;                                                                  \
                    break;                                                                         \
                }                                                                                  \
            }                                                                                      \
            if (!contains) {                                                                       \
                printf("(%s) Deinflection test for '%s' FAILED!\n Deinflections are:\n", __func__, \
                       *wordptr);                                                                  \
                for (size_t i = 0; i < buf_size(deinfs); i++) {                                    \
                    printf("%.*s\n", (int)deinfs[i].len, (char *)deinfs[i].s);                     \
                }                                                                                  \
                return 0;                                                                          \
            }                                                                                      \
            frees8buffer(&deinfs);                                                                 \
        }                                                                                          \
    } while (0)

static int test_deinflections(void) {
    /* -- Ichidan verbs -- */
    DEINFLECT("信じる", "信じない", "信じます", "信じません", "信じた", "信じなかった",
              "信じました", "信じませんでした", "信じて", "信じなくて", "信じられる",
              "信じられない", "信じられる", "信じられない", "信じさせる", "信じさせない",
              "信じさせられる", "信じさせられない", "信じろ");

    /* --  Godan verbs -- */
    /* ku-ending */
    DEINFLECT("叩く", "叩かない", "叩きます", "叩きません", "叩いた", "叩かなかった", "叩きました",
              "叩きませんでした", "叩いて", "叩かなくて", "叩ける", "叩けない", "叩かれる",
              "叩かれない", "叩かせる", "叩かせない", "叩かせられる", "叩かせられない", "叩け");
    /* u-ending */
    DEINFLECT("買う", "買わない", "買います", "買いません", "買った", "買わなかった", "買いました",
              "買いませんでした", "買って", "買わなくて", "買える", "買えない", "買われる",
              "買われない", "買わせる", "買わせない", "買わせられる", "買わせられない", "買え");
    /* gu-ending */
    DEINFLECT("泳ぐ", "泳がない", "泳ぎます", "泳ぎません", "泳いだ", "泳がなかった", "泳ぎました",
              "泳ぎませんでした", "泳いで", "泳がなくて", "泳げる", "泳げない", "泳がれる",
              "泳がれない", "泳がせる", "泳がせない", "泳がせられる", "泳がせられない", "泳げ");
    /* mu-ending */
    DEINFLECT("読む", "読まない", "読みます", "読みません", "読んだ", "読まなかった", "読みました",
              "読みませんでした", "読んで", "読まなくて", "読める", "読めない", "読まれる",
              "読まれない", "読ませる", "読ませない", "読ませられる", "読ませられない", "読め");
    /* bu-ending */
    DEINFLECT("遊ぶ", "遊べ");
    /* tsu-ending */
    DEINFLECT("勝つ", "勝て");

    /* -- i-adjectives -- */
    DEINFLECT("軽い", "軽くない", "軽かった", "軽くなかった");

    return 1;
}

#define TEST(call)                                                                                 \
    do {                                                                                           \
        if ((call) == 0) {                                                                         \
            exit(EXIT_FAILURE);                                                                    \
        } else {                                                                                   \
            printf("%s: Success\n", #call);                                                        \
        }                                                                                          \
    } while (0)

int main(int argc, char **argv) {
    TEST(test_kana_conversion());
    TEST(test_deinflections());

    return EXIT_SUCCESS;
}
