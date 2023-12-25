#include <stdio.h>
#include <glib.h>
#include <locale.h>

#include "unistr.h"

#define Stopif(assertion, error_action, ...)          \
	if (assertion) {                              \
		fprintf(stderr, __VA_ARGS__);         \
		fprintf(stderr, "\n");                \
		error_action;                         \
	}

unistr*
init_unistr(const char *str)
{
	return g_string_new(str);
	/* unistr *us = malloc(sizeof(unistr)); */

	/* GError *e = NULL; */
	/* setlocale(LC_ALL, ""); */
	/* char *str_utf8 = g_locale_to_utf8(str, -1, NULL, NULL, &e); */
	/* Stopif(!g_utf8_validate(str_utf8, -1, NULL), free(str_utf8); free(us); return NULL, */
	/*        "ERROR: Word could not be converted to a valid UTF-8 string."); */

	/* us->str = str_utf8; */
	/* us->len = g_utf8_strlen(us->str, -1); */
	/* us->byte_len = strlen(str); */

	/* return us; */
}

void
unistr_free(unistr *us)
{
	g_string_free(us, TRUE);
	/* g_free(us->str); */
	/* free(us); */
}

/**
 * @input: a validated utf8-string .
 *
 * Returns: Pointer to the start of the nth to last unicode character. Counting from 0
 * If n is too big, returns last character
 */
const char *
char_at_pos(const char *input, size_t n)
{
	const char *p = input;
	for (int i = 0; i < n && *p != '\0'; i++)
		p = g_utf8_next_char(p);
	return p;
}

/**
 * unistr_replace_ending:
 * @str: The UTF-8 encoded string to replace with
 * @len: The length of the ending (in bytes) that should be replaced
 *
 * Replaces the last @len bytes of characters with @str. Assumes replacement is smaller than ending.
 * NO ERROR CHECKING.
 *
 * Returns: The newly allocated string with replaced ending.
 */
char *
unistr_replace_ending(const unistr* word, const char *replacement, size_t len_ending)
{
	Stopif(len_ending > word->len, return NULL, 
	    "ERROR: Received a length greater than word length in unistr_replace_ending.");

	char *retstr = malloc(word->len + 1);
	char *end = mempcpy(retstr, word->str, word->len - len_ending);
	strcpy(end, replacement);

	return retstr;

	/* GString *gword = g_string_sized_new(word->byte_len + 5); // Try to avoid resizing */
	/* const char *start_ending = char_at_pos(word->str, word->len - len); */

	/* g_string_append_len(gword, word->str, start_ending - word->str); */
	/* g_string_append(gword, str); */

	/* GError *e = NULL; */
	/* char *str_locale = g_locale_from_utf8(gword->str, gword->len, NULL, NULL, &e); */
	/* g_string_free(gword, TRUE); */

	/* return str_locale; */
	/* return g_string_free_and_steal(gword); */
}

char *
get_ptr_to_char_before(unistr *word, size_t len_ending)
{
    char *chr_after = word->str + len_ending;
    return chr_after - 3; 
    // Might be dangerous. However we know that data is UTF-8 encoded and that japanese characters are 3 bytes long,
    // so should be fine.
    // NO ERROR CHECKING.
}

int
unichar_at_equals(unistr *word, size_t pos, const char *str)
{
	const char *charat = char_at_pos(word->str, pos);
	return (strncmp(charat, str, strlen(str)) == 0);
}
