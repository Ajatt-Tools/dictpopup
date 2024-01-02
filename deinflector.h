/*
 * @word: The japanese word to be deinflected
 *
 * Contains all intermediate steps, e.g. してしまった -> してしまう, して, する
 *
 * Returns: A null terminated array with possible deinflections. Array needs to be freed with g_strfreev() for example.
 */
char **deinflect(char *word);
