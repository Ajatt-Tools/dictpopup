/*
 * @word: The japanese word to be deinflected
 *
 * Contains all intermediate steps, e.g. してしまった -> してしまう, して, する
 *
 * Returns: A null terminated array with possible deinflections. Array needs to be freed with g_strfreev() for example.
 */
s8** deinflect(s8 word);

/*
 * Tries to give a hiragana conversion of @input using MeCab.
 *
 * Returns: A newly allocated string containing the conversion.
 */
char* kanji2hira(s8 input);
