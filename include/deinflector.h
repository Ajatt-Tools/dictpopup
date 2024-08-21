#include "utils/util.h"

#include <utils/str.h>

/*
 * @word: The japanese word to be deinflected
 *
 * Contains all intermediate steps, e.g. してしまった -> してしまう -> して ->
 * する
 *
 * Returns: An s8* buffer (see buf.h) containing the deinflections.
 * The caller of the function takes ownership of the data, and is responsible for freeing it.
 */
_deallocator_(s8_buf_free) s8Buf deinflect(s8 word);

/*
 * Tries to give a hiragana conversion of @input using MeCab.
 *
 * Returns: A newly allocated string containing the conversion.
 */
s8 kanji2hira(s8 input);
