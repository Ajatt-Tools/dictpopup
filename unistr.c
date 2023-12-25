#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "unistr.h"

#define Stopif(assertion, error_action, ...)          \
	if (assertion) {                              \
		fprintf(stderr, __VA_ARGS__);         \
		fprintf(stderr, "\n");                \
		error_action;                         \
	}

/* Assumes string is already UTF-8 encoded! */
void
unistr_init(unistr *us, const char *str)
{
	us->str = str;
	us->len = strlen(str);
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
}

const char *
get_ptr_to_char_before(unistr *word, size_t len_ending)
{
    const char *chr_after = word->str + len_ending;
    return chr_after - 3;
    // Might be dangerous. However we know that data is UTF-8 encoded and that japanese characters are 3 bytes long,
    // so should be fine. NO ERROR CHECKING though.
}
