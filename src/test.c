#include <stdio.h>
#include "util.h"
#include "deinflector.h"

#define passed(cond) ((cond) ? "passed" : "not passed")

int
main(int argc, char** argv)
{
    const s8 all_kata = S("アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲン"
			   "ガギグゲゴザジズゼゾダヂヅデド"        "バビブベボ"
			                                           "パピプペポ");
    const s8 all_hira = S("あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをん"
			   "がぎぐげござじずぜぞだぢづでど"        "ばびぶべぼ"
			                                           "ぱぴぷぺぽ");
    s8 converted_hira = s8dup(all_kata);
    kata2hira(all_kata);
    printf("kata2hira(): %s\n", passed(s8equals(converted_hira, all_hira))); // Still needs fuzzy testing though
    frees8(&converted_hira);


    const s8 expected_pth = S("This is/a/test, if/building/paths/works/as expected");
    s8 built_pth = buildpath(S("This is"), S("a"), S("test, if"), S("building"), S("paths"), S("works"), S("as expected"));
    printf("buildpath(): %s\n", passed(s8equals(expected_pth, built_pth)));
    frees8(&built_pth);
}
