#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <locale.h>


#define Stopif(assertion, error_action, ...)      \
	if (assertion) {                           \
		error_action;                         \
	}

#define comparechr(string, char) \
	!strncmp(string, char, strlen(char))

/*
   Input: Validated utf8-string.
   Output: Pointer to the start of the nth to last unicode character. Counting from 0
   If n is too big, returns last character
 */
const char *
nth_char(const char *input, size_t n)
{
	const char *p = input;
	for (int i = 0; i < n && *p != '\0'; i++)
		p = g_utf8_next_char(p);
	return p;
}

/*
   Replaces char at position pos with c
 */
// TODO: Seems rather inefficient
int
add_replace(GPtrArray *deinflections, const char *word, const char *c, size_t pos, int keepending)
{
	size_t wordlen = strlen(word);
	Stopif(pos > wordlen, return 0, "ERROR: Recieved a replace position outside of string.");

	GString *gword = g_string_sized_new(wordlen + 2); // Avoid resizing if c takes more bytes

	const char *startc = nth_char(word, pos);
	g_string_append_len(gword, word, startc - word);
	g_string_append(gword, c);
	if (keepending)
		g_string_append(gword, g_utf8_next_char(startc));

	g_ptr_array_add(deinflections, g_string_free_and_steal(gword));
	return 1;
}

int
add_truncate(GPtrArray *deinflections, const char *word, glong len, size_t num_chars)
{
	const char *end = nth_char(word, len - num_chars);
	char *truncatedstr = g_strndup(word, end - word);

	g_ptr_array_add(deinflections, truncatedstr);
	return 1;
}

/*
   Converts the character at pos from a to u form if aform == TRUE and from i to u form else.
   The result is added to the deinflections array.
*/
int
itou_atou_form(GPtrArray *deinflections, const char *word, glong len, size_t pos, int aform)
{
	if (pos < 0)
	    return 0;

	const char *aforms[] = { "さ", "か", "が", "ば", "た", "ま", "わ", "な", "ら" };
	const char *iforms[] = { "し", "き", "ぎ", "び", "ち", "み", "い", "に", "り" };
	const char *uforms[] = { "す", "く", "ぐ", "ぶ", "つ", "む", "う", "ぬ", "る" };
	int num = sizeof(uforms) / sizeof(uforms[0]); // expects same length

	const char *charatpos = nth_char(word, pos);
	const char **forms = aform? aforms : iforms;

	int i = 0;
	while (!comparechr(charatpos, forms[i]) && i < num)
		i++;

	if (i < num)
		add_replace(deinflections, word, uforms[i], pos, 0);
	if (!aform || i >= num) // For iform orig can be る-form or not, e.g. 生きます
		add_replace(deinflections, word, "る", pos+1, 0);

	return 1;
}

int
te_form_prev_char(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	/* add_replace(deinflections, word, "る", len - 1, 0); */

	if (comparechr(last2, "し"))
		if (len == 2)
			return add_replace(deinflections, word, "する", len - 2, 0);
		else
			return add_replace(deinflections, word, "す", len - 2, 0);
	else if (comparechr(last2, "い"))
		return add_replace(deinflections, word, "く", len - 2, 0);
	else if (comparechr(last2, "ん"))
	{
		add_replace(deinflections, word, "す", len - 2, 0);
		add_replace(deinflections, word, "ぶ", len - 2, 0);
		add_replace(deinflections, word, "ぬ", len - 2, 0);
		return 1;
	}
	else if (comparechr(last2, "く")) /* e.g. 行かなくて, not neseccary actually */
		return add_replace(deinflections, word, "い", len - 2, 0);
	else if (comparechr(last2, "っ"))
	{
		if (last3 && !strncmp(last3, "行", strlen("行")))
			return add_replace(deinflections, word, "く", len - 2, 0);
		else
		{
			add_replace(deinflections, word, "る", len - 2, 0);
			add_replace(deinflections, word, "う", len - 2, 0);
			add_replace(deinflections, word, "つ", len - 2, 0);
			return 1;
		}
	}

	return 0;
}

int
de_form_prev_char(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	if (comparechr(last2, "い"))
		add_replace(deinflections, word, "ぐ", len - 2, 0);
	else if (comparechr(last2, "ん"))
	{
		add_replace(deinflections, word, "む", len - 2, 0);
		add_replace(deinflections, word, "ぶ", len - 2, 0);
		add_replace(deinflections, word, "ぬ", len - 2, 0);
		return 1;
	}

	return 0;
}

int
check_te_form(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	if (!strcmp(last1, "て"))
	{
		return te_form_prev_char(deinflections, word, len, last3, last2, last1);
	}
	if (!strcmp(last1, "で"))
		return de_form_prev_char(deinflections, word, len, last3, last2, last1);
	return 0;
}

int
check_past_form(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	if (!strcmp(last1, "た"))
		return te_form_prev_char(deinflections, word, len, last3, last2, last1);
	if (!strcmp(last1, "だ"))
		return de_form_prev_char(deinflections, word, len, last3, last2, last1);
	return 0;
}

int
check_masu_form(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	if (!strcmp(last2, "ます"))
		return itou_atou_form(deinflections, word, len, len - 3, 0);
	else if (last3 && (!strcmp(last3, "ません") || !strcmp(last3, "ました")))
		return itou_atou_form(deinflections, word, len, len - 4, 0);
	return 0;
}

void
kanjify(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	if (comparechr(word, "ご") || comparechr(word, "お"))
		add_replace(deinflections, word, "御", 0, 1);
	if (!strcmp(last2, "なし") || !strcmp(last2, "ない"))
		add_replace(deinflections, word, "無", len - 2, 1);
	if (last3 && (!strcmp(last3, "かかり")
		      || !strcmp(last3, "がかり")
		      || !strcmp(last3, "かかる")
		      || !strcmp(last3, "かかる")))
		add_replace(deinflections, word, "掛", len - 3, 1);
}

int
check_shimau_form(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	if (last3 && !strcmp(last3, "しまう"))
		return add_truncate(deinflections, word, len, 3);

	return 0;
}

int
check_passive_causative_form(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	if (last3 && (!strcmp(last3, "られる") || !strcmp(last3, "させる")))
		return add_replace(deinflections, word, "る", len - 3, 0);
	else if (!strcmp(last2, "れる") || !strcmp(last2, "せる"))
		return itou_atou_form(deinflections, word, len, len - 3, 1);

	return 0;
}

int
check_adjective(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	/* 過去形 */
	if (last3 && !strcmp(last3, "かった"))
	{
		add_replace(deinflections, word, "い", len - 3, 0);
		return 0; /* No return, since can still be a verb, e.g. 授かった */
	}
	else if (!strcmp(last1, "く") || !strcmp(last1, "さ"))
		return add_replace(deinflections, word, "い", len - 1, 0);

	return 0;
}

int
check_volitional(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	if (!strcmp(last2, "たい"))
		return itou_atou_form(deinflections, word, len, len - 3, 0);
	return 0;
}

int
check_negation(GPtrArray *deinflections, const char *word, size_t len, const char *last3, const char *last2, const char *last1)
{
	if (!strcmp(last2, "ない"))
		return itou_atou_form(deinflections, word, len, len - 3, 1);
	return 0;
}

void
deinflect_one_iter(GPtrArray *deinflections, char *word_utf8)
{
	glong len = g_utf8_strlen(word_utf8, -1);
	Stopif(len < 2, return );

	const char *last3 = len >=3 ? nth_char(word_utf8, len-3) : NULL;
	const char *last2 = last3 == NULL ? word_utf8 : g_utf8_next_char(last3);
	const char *last1 = g_utf8_next_char(last2);

	kanjify(deinflections, word_utf8, len, last3, last2, last1);

	check_shimau_form(deinflections, word_utf8, len, last3, last2, last1);

	check_adjective(deinflections, word_utf8, len, last3, last2, last1);

	check_te_form(deinflections, word_utf8, len, last3, last2, last1);

	check_past_form(deinflections, word_utf8, len, last3, last2, last1);

	check_masu_form(deinflections, word_utf8, len, last3, last2, last1);

	check_passive_causative_form(deinflections, word_utf8, len, last3, last2, last1);

	check_volitional(deinflections, word_utf8, len, last3, last2, last1);

	check_negation(deinflections, word_utf8, len, last3, last2, last1);

}

// TODO: Convert back to users locale. (with g_locale_from_utf8)
char const*
deinflect(GPtrArray *deinflections, const char *word)
{
	GError *e = NULL;
	setlocale(LC_ALL, "");
	char *word_utf8 = g_locale_to_utf8(word, -1, NULL, NULL, &e);
	Stopif(!g_utf8_validate(word_utf8, -1, NULL), free(word_utf8);
	       return "ERROR: Word could not be converted to a valid UTF-8 string.");

	deinflect_one_iter(deinflections, word_utf8);

	size_t num_deinfs = deinflections->len;
	for (int i = 0; i < num_deinfs; i++)
	{
		deinflect_one_iter(deinflections, g_ptr_array_index(deinflections, i));
		num_deinfs = deinflections->len;
	}

	g_free(word_utf8);
	return NULL;
}
