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
    const size_t single_chr_len = strlen("あ");
    return len_ending + single_chr_len > word->len ? NULL
      : word->str + word->len - len_ending - single_chr_len;
    // Might be dangerous if string is not UTF-8 encoded
}
