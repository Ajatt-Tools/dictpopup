#include <stdio.h>

#include "deinflector.h"
#include "dictpopup.h"
#include "util.h"

#define passed(cond) ((cond) ? "passed" : "not passed")

static int test_kana_conversion(void) {
    // TODO: Test if other characters are left untouched?
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

static void dump_deinfs(s8 *deinfs) {
    puts("Deinflections are:");
    for (size_t i = 0; i < buf_size(deinfs); i++) {
        printf("%.*s\n", (int)deinfs[i].len, (char *)deinfs[i].s);
    }
}

// Checks if expected_deinf is contained in the deinflections array obtained from the other args
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
                printf("(%s) '%s' doesn't get deinflected to '%s'!\n", __func__, *wordptr,         \
                       expected_deinf);                                                            \
                dump_deinfs(deinfs);                                                               \
                return 0;                                                                          \
            }                                                                                      \
            frees8buffer(&deinfs);                                                                 \
        }                                                                                          \
    } while (0)

// Checks that nodeinf is not contained in the deinflection of word
#define NODEINFLECTION(nodeinf, word)                                                              \
    do {                                                                                           \
        s8 *deinfs = deinflect(fromcstr_(word));                                                   \
        for (size_t i = 0; i < buf_size(deinfs); i++) {                                            \
            if (s8equals(S(nodeinf), deinfs[i])) {                                                 \
                printf("(%s) Deinflection of '%s' contains '%s'!\n", __func__, word, nodeinf);     \
                dump_deinfs(deinfs);                                                               \
                return 0;                                                                          \
            }                                                                                      \
        }                                                                                          \
        frees8buffer(&deinfs);                                                                     \
    } while (0)

static int test_deinflections(void) {
    /* -- Ichidan verbs -- */
    DEINFLECT("信じる", "信じない", "信じます", "信じません", "信じた", "信じなかった",
              "信じました", "信じませんでした", "信じて", "信じなくて", "信じられる",
              "信じられない", "信じられない", "信じさせる", "信じさせない", "信じさせられる",
              "信じさせられない", "信じろ", "信じ", "信じよう");

    DEINFLECT("生きる", "生きない", "生きます", "生きません", "生きた", "生きなかった",
              "生きました", "生きませんでした", "生きて", "生きなくて", "生きられる",
              "生きられない", "生きられない", "生きさせる", "生きさせない", "生きさせられる",
              "生きさせられない", "生きろ", "生き", "生きよう");

    /* --  Godan verbs -- */
    /* ku-ending */
    DEINFLECT("叩く", "叩かない", "叩きます", "叩きません", "叩いた", "叩かなかった", "叩きました",
              "叩きませんでした", "叩いて", "叩かなくて", "叩ける", "叩けない", "叩かれる",
              "叩かれない", "叩かせる", "叩かせない", "叩かせられる", "叩かせられない", "叩け",
              "叩き", "叩こう");
    /* u-ending */
    DEINFLECT("買う", "買わない", "買います", "買いません", "買った", "買わなかった", "買いました",
              "買いませんでした", "買って", "買わなくて", "買える", "買えない", "買われる",
              "買われない", "買わせる", "買わせない", "買わせられる", "買わせられない", "買え",
              "買い", "買おう");
    /* gu-ending */
    DEINFLECT("泳ぐ", "泳がない", "泳ぎます", "泳ぎません", "泳いだ", "泳がなかった", "泳ぎました",
              "泳ぎませんでした", "泳いで", "泳がなくて", "泳げる", "泳げない", "泳がれる",
              "泳がれない", "泳がせる", "泳がせない", "泳がせられる", "泳がせられない", "泳げ",
              "泳ぎ", "泳ごう");
    /* mu-ending */
    DEINFLECT("読む", "読まない", "読みます", "読みません", "読んだ", "読まなかった", "読みました",
              "読みませんでした", "読んで", "読まなくて", "読める", "読めない", "読まれる",
              "読まれない", "読ませる", "読ませない", "読ませられる", "読ませられない", "読め",
              "読み", "読もう");
    /* bu-ending */
    DEINFLECT("遊ぶ", "遊ばない", "遊びます", "遊びません", "遊んだ", "遊ばなかった", "遊びました",
              "遊びませんでした", "遊んで", "遊ばなくて", "遊べる", "遊べない", "遊ばれる",
              "遊ばれない", "遊ばせる", "遊ばせない", "遊ばせられる", "遊ばせられない", "遊べ",
              "遊び", "遊ぼう");
    /* tsu-ending */
    DEINFLECT("勝つ", "勝たない", "勝ちます", "勝ちません", "勝った", "勝たなかった", "勝ちました",
              "勝ちませんでした", "勝って", "勝たなくて", "勝てる", "勝てない", "勝たれる",
              "勝たれない", "勝たせる", "勝たせない", "勝たせられる", "勝たせられない", "勝て",
              "勝ち", "勝とう");

    /* -- i-adjectives -- */
    DEINFLECT("軽い", "軽くない", "軽かった", "軽くなかった");

    /* -- katakana -- */
    DEINFLECT("きれい", "キレイ");
    DEINFLECT("おもしろい", "オモシロイ");
    DEINFLECT("ضüщшерт1234asdfあいんしゅたいんhjkl6789уиопö",
              "ضüщшерт1234asdfアインシュタインhjkl6789уиопö");

    /* -- ?? -- */
    DEINFLECT("目する", "目した");

    /* たい */
    DEINFLECT("遊ぶ", "遊びたい", "遊びたかろう", "遊びたかった", "遊びたければ");
    DEINFLECT("食べる", "食べたい");
    DEINFLECT("旅行する", "旅行したい");
    DEINFLECT("会う", "会わせたい");
    DEINFLECT("寝る", "寝させたい");
    DEINFLECT("思う", "思われたい");
    DEINFLECT("褒める", "褒められたい");

    /* たがる */
    DEINFLECT("遊ぶ", "遊びたがる", "遊びたがらない", "遊びたがります", "遊びたがった",
              "遊びたがれば");

    /* -- negations -- */
    DEINFLECT("食べる", "食べず");
    DEINFLECT("行く", "行かず");
    DEINFLECT("知る", "知らん");
    DEINFLECT("泣く", "泣かぬ");
    DEINFLECT("信じる", "信じぬ");
    DEINFLECT("落ちる", "落ちまい");
    DEINFLECT("消える", "消えまい");
    DEINFLECT("知る", "知らせまい");
    DEINFLECT("食べる", "食べさせまい");
    DEINFLECT("言う", "言われまい");

    /* -- should not contain -- */
    NODEINFLECTION("白い", "白ける");
    /* NODEINFLECTION("やった", "やつ"); // Not implemented yet */

    return 1;
}

static i32 test_dictionary_lookup(void) {
    dictpopup_t d = {0};

    d.pe.lookup = S("面白い");
    dictentry *dict = create_dictionary(&d);
    // TODO
    dictionary_free(&dict);
    return 1;
}

#define TEST(call)                                                                                 \
    do {                                                                                           \
        int _rc = call;                                                                            \
        if (_rc == 0) {                                                                            \
            exit(EXIT_FAILURE);                                                                    \
        } else {                                                                                   \
            printf("%s: Success\n", #call);                                      \
        }                                                                                          \
    } while (0)

int main(int argc, char **argv) {
    TEST(test_kana_conversion());
    TEST(test_deinflections());

    return EXIT_SUCCESS;
}
