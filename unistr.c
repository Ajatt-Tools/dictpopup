#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <locale.h>
#include <glib.h>

#include "unistr.h"


unistr*
unistr_new(const char *str)
{
	unistr *us = malloc(sizeof(unistr));

	// TODO: Check necessity of commented code
	/* GError *e = NULL; */
	/* setlocale(LC_ALL, ""); */
	/* char *str_utf8 = g_locale_to_utf8(str, -1, NULL, NULL, &e); */

	/* if (!g_utf8_validate(str_utf8, -1, NULL)) */
	/* 	g_warning("Could not convert word to a valid UTF-8 string."); */

	*us = (unistr) { .str=str, .len=strlen(str) };

	return us;
}

void
unistr_free(unistr *us)
{
	/* g_free(us->str); */
	free(us);
}

/**
 * unistr_replace_ending:
 * @word: The word whose ending should be replaced
 * @replacement: The UTF-8 encoded string the ending should be replaced with
 * @len_ending: The length of the ending (in bytes) that should be replaced
 *
 * Replaces the last @len_ending bytes of @word with @str.
 *
 * Returns: The newly allocated string with replaced ending.
 */
char *
unistr_replace_ending(const unistr* word, const char *replacement, size_t len_ending)
{
	char *retstr = malloc(word->len - len_ending + strlen(replacement) + 1);
	char *end = mempcpy(retstr, word->str, word->len - len_ending);
	strcpy(end, replacement);

	return retstr;
}
