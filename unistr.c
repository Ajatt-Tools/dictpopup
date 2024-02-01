#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <locale.h>
#include <glib.h>

#include "unistr.h"


unistr*
unistr_new(const char *str)
{
	unistr* us = g_new(unistr, 1);

	*us = (unistr) { .str = str, .len = strlen(str) };
	return us;
}

void
unistr_free(unistr *us)
{
	g_free(us);
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
	char* retstr = g_malloc(word->len - len_ending + strlen(replacement) + 1);

	memcpy(retstr, word->str, word->len - len_ending);
	strcpy(retstr + word->len - len_ending, replacement);

	return retstr;
}
