#include "util.h"

/*
 * @word: The japanese word to be deinflected
 *
 * Contains all intermediate steps, e.g. してしまった -> してしまう -> して ->
 * する
 *
 * Returns: An s8* buffer (see buf.h) containing the deinflections.
 * The caller of the function takes ownership of the data, and is responsible for freeing it.
 */
_deallocator_(frees8buffer) s8 *deinflect(s8 word);

/*
 * Tries to give a hiragana conversion of @input using MeCab.
 *
 * Returns: A newly allocated string containing the conversion.
 */
s8 kanji2hira(s8 input);

/*
 * Converts all katakana characters in @kata_in into its hiragana counterparts
 * (inplace).
 */
void kata2hira(s8 kata_in);
