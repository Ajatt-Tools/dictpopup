
#include "deinflector/kata2hira.h"
#include "utils/utf8.h"

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