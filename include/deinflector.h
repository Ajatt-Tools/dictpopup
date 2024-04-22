#include "util.h"

/*
 * @word: The japanese word to be deinflected
 *
 * Contains all intermediate steps, e.g. してしまった -> してしまう -> して -> する
 *
 * Returns: An s8* buffer containing the deinflections. All s8 strings need to be freed as well as te buffer with buf_free
 */
s8* deinflect(s8 word);

/*
 * Tries to give a hiragana conversion of @input using MeCab.
 *
 * Returns: A newly allocated string containing the conversion.
 */
s8 kanji2hira(s8 input);

/*
 * Converts all katakana characters in @kata_in into its hiragana counterparts (inplace).
 */
void kata2hira(s8 kata_in);
