#include "deinflector.h"
#include "utils/util.h"
#include <cgreen/cgreen.h>

TestSuite *deinflector_tests(void);

Describe(Deinflector);
BeforeEach(Deinflector) {
}
AfterEach(Deinflector) {
}

Ensure(Deinflector, converts_kata_to_hira) {
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
    int comparison = s8equals(converted_hira, all_hira);
    assert_that(comparison, is_equal_to(1));
}

static bool buffer_contains(s8 *buffer, s8 word) {
    bool contains = false;
    for (size_t i = 0; i < buf_size(buffer); i++) {
        if (s8equals(buffer[i], word)) {
            contains = true;
            break;
        }
    }
    return contains;
}

static void buffer_dump(s8 *buffer) {
    for (size_t i = 0; i < buf_size(buffer); i++) {
        printf("%s\n", (char *)buffer[i].s);
    }
}

#define DEINFLECT(expected_deinf, ...)                                                             \
    do {                                                                                           \
        char **words = (char *[]){__VA_ARGS__, NULL};                                              \
        for (char **wordptr = words; *wordptr != NULL; wordptr++) {                                \
            _drop_(frees8buffer) s8 *deinfs = deinflect(fromcstr_(*wordptr));                      \
            bool contains = buffer_contains(deinfs, S(expected_deinf));                            \
            if (!contains) {                                                                       \
                printf("Expected '%s' to be deinflected to '%s'\n", *wordptr, expected_deinf);     \
                buffer_dump(deinfs);                                                               \
            }                                                                                      \
            assert_that(contains, is_true);                                                        \
        }                                                                                          \
    } while (0)

Ensure(Deinflector, deinflects_kuru_kanji) {
    DEINFLECT("来る", "来ない", "来ます", "来ません", "来た", "来なかった", "来ました", "来て",
              "来なくて", "来られない", "来られる", "来られない", "来させる", "来させない",
              "来させられる", "来させられない", "来い");
}

Ensure(Deinflector, deinflects_kuru_hira) {
    DEINFLECT("くる", "こない", "きます", "きません", "きた", "こなかった", "きました", "きて",
              "こなくて", "こられる", "こられない", "こられない", "こさせる", "こさせない",
              "こさせられる", "こさせられない", "こい");
}

Ensure(Deinflector, deinflects_ichidan) {
    DEINFLECT("信じる", "信じない", "信じます", "信じません", "信じた", "信じなかった",
              "信じました", "信じませんでした", "信じて", "信じなくて", "信じられる",
              "信じられない", "信じられない", "信じさせる", "信じさせない", "信じさせられる",
              "信じさせられない", "信じろ", "信じ", "信じよう");

    DEINFLECT("生きる", "生きない", "生きます", "生きません", "生きた", "生きなかった",
              "生きました", "生きませんでした", "生きて", "生きなくて", "生きられる",
              "生きられない", "生きられない", "生きさせる", "生きさせない", "生きさせられる",
              "生きさせられない", "生きろ", "生き", "生きよう");
}

Ensure(Deinflector, deinflects_godan_ru) {
    DEINFLECT("売る", "売らない", "売ります", "売りません", "売った", "売らなかった", "売りました",
              "売りませんでした", "売って", "売らなくて", "売れる", "売れない", "売られる",
              "売られない", "売らせる", "売らせない", "売らせられる", "売らせられない", "売れ",
              "売り", "売ろう");
}

Ensure(Deinflector, deinflects_godan_mu) {
    DEINFLECT("読む", "読まない", "読みます", "読みません", "読んだ", "読まなかった", "読みました",
              "読みませんでした", "読んで", "読まなくて", "読める", "読めない", "読まれる",
              "読まれない", "読ませる", "読ませない", "読ませられる", "読ませられない", "読め",
              "読み", "読もう");
}

Ensure(Deinflector, deinflects_godan_bu) {
    DEINFLECT("遊ぶ", "遊ばない", "遊びます", "遊びません", "遊んだ", "遊ばなかった", "遊びました",
              "遊びませんでした", "遊んで", "遊ばなくて", "遊べる", "遊べない", "遊ばれる",
              "遊ばれない", "遊ばせる", "遊ばせない", "遊ばせられる", "遊ばせられない", "遊べ",
              "遊び", "遊ぼう");
}

Ensure(Deinflector, deinflects_godan_tsu) {
    DEINFLECT("勝つ", "勝たない", "勝ちます", "勝ちません", "勝った", "勝たなかった", "勝ちました",
              "勝ちませんでした", "勝って", "勝たなくて", "勝てる", "勝てない", "勝たれる",
              "勝たれない", "勝たせる", "勝たせない", "勝たせられる", "勝たせられない", "勝て",
              "勝ち", "勝とう");
}

Ensure(Deinflector, deinflects_godan_su) {
    DEINFLECT("誑かす", "誑かさない", "誑かします", "誑かしません", "誑かした", "誑かさなかった",
              "誑かしました", "誑かしませんでした", "誑かして", "誑かさなくて", "誑かせる",
              "誑かせない", "誑かされる", "誑かされない", "誑かさせる", "誑かさせない",
              "誑かさせられる", "誑かさせられない", "誑かせ", "誑かし", "誑かそう");
}

Ensure(Deinflector, deinflects_godan_ku) {
    DEINFLECT("叩く", "叩かない", "叩きます", "叩きません", "叩いた", "叩かなかった", "叩きました",
              "叩きませんでした", "叩いて", "叩かなくて", "叩ける", "叩けない", "叩かれる",
              "叩かれない", "叩かせる", "叩かせない", "叩かせられる", "叩かせられない", "叩け",
              "叩き", "叩こう");
}

Ensure(Deinflector, deinflects_godan_gu) {
    DEINFLECT("泳ぐ", "泳がない", "泳ぎます", "泳ぎません", "泳いだ", "泳がなかった", "泳ぎました",
              "泳ぎませんでした", "泳いで", "泳がなくて", "泳げる", "泳げない", "泳がれる",
              "泳がれない", "泳がせる", "泳がせない", "泳がせられる", "泳がせられない", "泳げ",
              "泳ぎ", "泳ごう");
}

Ensure(Deinflector, deinflects_godan_u) {
    DEINFLECT("買う", "買わない", "買います", "買いません", "買った", "買わなかった", "買いました",
              "買いませんでした", "買って", "買わなくて", "買える", "買えない", "買われる",
              "買われない", "買わせる", "買わせない", "買わせられる", "買わせられない", "買え",
              "買い", "買おう");
}

Ensure(Deinflector, deinflects_i_adjectives) {
    DEINFLECT("軽い", "軽くない", "軽かった", "軽くなかった");
}

Ensure(Deinflector, deinflects_kata2hira) {
    DEINFLECT("きれい", "キレイ");
    DEINFLECT("おもしろい", "オモシロイ");
    DEINFLECT("ضüщшерт1234asdfあいんしゅたいんhjkl6789уиопö",
              "ضüщшерт1234asdfアインシュタインhjkl6789уиопö");
}

Ensure(Deinflector, deinflects_negations) {
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
}

Ensure(Deinflector, deinflects_tai) {
    DEINFLECT("遊ぶ", "遊びたい", "遊びたかった", "遊びたければ");
    DEINFLECT("食べる", "食べたい");
    DEINFLECT("旅行する", "旅行したい");
    DEINFLECT("会う", "会わせたい");
    DEINFLECT("寝る", "寝させたい");
    DEINFLECT("思う", "思われたい");
    DEINFLECT("褒める", "褒められたい");
}

Ensure(Deinflector, deinflects_tagaru) {
    DEINFLECT("遊ぶ", "遊びたがる", "遊びたがらない", "遊びたがります", "遊びたがった",
              "遊びたがれば");
}

Ensure(Deinflector, deinflects_shita) {
    DEINFLECT("目する", "目した");
}

Ensure(Deinflector, deinflects_nagara) {
    DEINFLECT("走る", "走りながら");
    DEINFLECT("攪拌する", "攪拌しながら");
    DEINFLECT("食べる", "食べながら");
    DEINFLECT("書く", "書きながら");
}

Ensure(Deinflector, does_not_contain) {
    _drop_(frees8buffer) s8 *deinfs = deinflect(S("白ける"));
    bool contains = buffer_contains(deinfs, S("白い"));
    assert_that(contains, is_false);
}

Ensure(Deinflector, deinflects_null) {
    s8 *deinfs = deinflect((s8){0});
    assert_that(deinfs, is_null);
}

Ensure(Deinflector, deinflects_small_string) {
    s8 *deinfs = deinflect(fromcstr_("a"));
    assert_that(deinfs, is_null);
}

Ensure(Deinflector, deinflects_adj_sugiru) {
    DEINFLECT("色濃い", "色濃すぎる");
}

Ensure(Deinflector, deinflects_suru_and_compounds) {
    DEINFLECT("する", "しない", "します", "しません", "した", "しなかった", "しました",
              "しませんでした", "して", "しなくて", "できる", "できない", "される",
              "されない", "させる", "させない", "させられる", "させられない", "しろ",
              "し", "しよう");

    DEINFLECT("勉強する", "勉強しない", "勉強します", "勉強しません", "勉強した",
              "勉強しなかった", "勉強しました", "勉強しませんでした", "勉強して",
              "勉強しなくて", "勉強できる", "勉強できない", "勉強される", "勉強されない",
              "勉強させる", "勉強させない", "勉強させられる", "勉強させられない",
              "勉強しろ", "勉強し", "勉強しよう");
}

Ensure(Deinflector, deinflects_potential_forms) {
    DEINFLECT("読む", "読める", "読めない", "読めます", "読めません", "読めた", "読めなかった",
              "読めました", "読めませんでした", "読めて", "読めなくて");
    DEINFLECT("食べる", "食べられる", "食べられない", "食べられます", "食べられません",
              "食べられた", "食べられなかった", "食べられました", "食べられませんでした",
              "食べられて", "食べられなくて");
}

Ensure(Deinflector, deinflects_causative_forms) {
    DEINFLECT("読む", "読ませる", "読ませない", "読ませます", "読ませません", "読ませた",
              "読ませなかった", "読ませました", "読ませませんでした", "読ませて", "読ませなくて");
    DEINFLECT("食べる", "食べさせる", "食べさせない", "食べさせます", "食べさせません",
              "食べさせた", "食べさせなかった", "食べさせました", "食べさせませんでした",
              "食べさせて", "食べさせなくて");
}

Ensure(Deinflector, deinflects_passive_forms) {
    DEINFLECT("読む", "読まれる", "読まれない", "読まれます", "読まれません", "読まれた",
              "読まれなかった", "読まれました", "読まれませんでした", "読まれて", "読まれなくて");
    DEINFLECT("食べる", "食べられる", "食べられない", "食べられます", "食べられません",
              "食べられた", "食べられなかった", "食べられました", "食べられませんでした",
              "食べられて", "食べられなくて");
}

Ensure(Deinflector, deinflects_te_form) {
    DEINFLECT("読む", "読んで", "読んでいる", "読んでいない", "読んでいます", "読んでいません",
              "読んでいた", "読んでいなかった", "読んでいました", "読んでいませんでした");
    DEINFLECT("食べる", "食べて", "食べている", "食べていない", "食べています", "食べていません",
              "食べていた", "食べていなかった", "食べていました", "食べていませんでした");
}

Ensure(Deinflector, deinflects_polite_forms) {
    DEINFLECT("読む", "読みます", "読みません", "読みました", "読みませんでした");
    DEINFLECT("食べる", "食べます", "食べません", "食べました", "食べませんでした");
}

Ensure(Deinflector, deinflects_conditional_forms) {
    DEINFLECT("読む", "読めば", "読まなければ", "読んだら", "読まなかったら");
    DEINFLECT("食べる", "食べれば", "食べなければ", "食べたら", "食べなかったら");
}

Ensure(Deinflector, deinflects_adverb_forms) {
    DEINFLECT("早い", "早く");
}

Ensure(Deinflector, deinflects_compound_verbs) {
    DEINFLECT("飛び込む", "飛び込まない", "飛び込みます", "飛び込みません", "飛び込んだ",
              "飛び込まなかった", "飛び込みました", "飛び込みませんでした", "飛び込んで",
              "飛び込まなくて", "飛び込める", "飛び込めない");
    DEINFLECT("書き始める", "書き始めない", "書き始めます", "書き始めません", "書き始めた",
              "書き始めなかった", "書き始めました", "書き始めませんでした", "書き始めて",
              "書き始めなくて", "書き始められる", "書き始められない");
}

TestSuite *deinflector_tests(void) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, Deinflector, converts_kata_to_hira);
    add_test_with_context(suite, Deinflector, deinflects_kuru_kanji);
    add_test_with_context(suite, Deinflector, deinflects_kuru_hira);
    add_test_with_context(suite, Deinflector, deinflects_ichidan);
    add_test_with_context(suite, Deinflector, deinflects_godan_ru);
    add_test_with_context(suite, Deinflector, deinflects_godan_mu);
    add_test_with_context(suite, Deinflector, deinflects_godan_bu);
    add_test_with_context(suite, Deinflector, deinflects_godan_tsu);
    add_test_with_context(suite, Deinflector, deinflects_godan_su);
    add_test_with_context(suite, Deinflector, deinflects_godan_ku);
    add_test_with_context(suite, Deinflector, deinflects_godan_gu);
    add_test_with_context(suite, Deinflector, deinflects_godan_u);
    add_test_with_context(suite, Deinflector, deinflects_i_adjectives);
    add_test_with_context(suite, Deinflector, deinflects_kata2hira);
    add_test_with_context(suite, Deinflector, deinflects_negations);
    add_test_with_context(suite, Deinflector, deinflects_tai);
    // add_test_with_context(suite, Deinflector, deinflects_tagaru);
    add_test_with_context(suite, Deinflector, deinflects_shita);
    // add_test_with_context(suite, Deinflector, deinflects_nagara);
    // add_test_with_context(suite, Deinflector, does_not_contain);
    add_test_with_context(suite, Deinflector, deinflects_null);
    add_test_with_context(suite, Deinflector, deinflects_small_string);
    add_test_with_context(suite, Deinflector, deinflects_adj_sugiru);
    add_test_with_context(suite, Deinflector, deinflects_suru_and_compounds);
    add_test_with_context(suite, Deinflector, deinflects_potential_forms);
    add_test_with_context(suite, Deinflector, deinflects_causative_forms);
    add_test_with_context(suite, Deinflector, deinflects_passive_forms);
    add_test_with_context(suite, Deinflector, deinflects_te_form);
    add_test_with_context(suite, Deinflector, deinflects_polite_forms);
    add_test_with_context(suite, Deinflector, deinflects_conditional_forms);
    add_test_with_context(suite, Deinflector, deinflects_adverb_forms);
    add_test_with_context(suite, Deinflector, deinflects_compound_verbs);
    return suite;
}