#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include "unistr.h"

GPtrArray *deinfs;

#define Stopif(assertion, error_action, ...)          \
	if (assertion) {                              \
		fprintf(stderr, __VA_ARGS__);         \
		fprintf(stderr, "\n");                \
		error_action;                         \
	}

/*
   Replaces char at position wordlen - pos with c and adds it to the deinflections array.
   e.g. add_replace(しまう, 1, 0) replaces う
 */
int
add_replace_ending(unistr* word, const char *c, size_t pos)
{
	char *replaced_str = unistr_replace_ending(word, c, pos);
	g_ptr_array_add(deinfs, replaced_str);

	return 1;
}

/*
   Truncates num_chars of unicode characters and adds it to the deinflections array.
 */
int
add_truncate(unistr* word, size_t num_chars)
{
	return add_replace_ending(word, "", num_chars);
}

/*
   Adds string to the deinflections array.
 */
int
add_str(const char* str)
{
	g_ptr_array_add(deinfs, strdup(str));
	return 1;
}


/*
 * Converts a word in い- or あ-form to the う-form.
 * Pos points to the (potential) character in い- or あ-form to be converted FROM BEHIND.
 * The result is added to the deinflections array.
 */
enum { itou, atou };

int
itou_atou_form(unistr *word, size_t pos, int conv_type)
{
	Stopif(pos < 0, return 0, "ERROR: Received an invalid position in itou_atou_form function.");

	const char *aforms[] = { "さ", "か", "が", "ば", "た", "ま", "わ", "な", "ら", NULL };
	const char *iforms[] = { "し", "き", "ぎ", "び", "ち", "み", "い", "に", "り", NULL };
	const char *uforms[] = { "す", "く", "ぐ", "ぶ", "つ", "む", "う", "ぬ", "る", NULL };

	const char **forms = (conv_type == atou) ? aforms : iforms;

	while (*forms && !unichar_at_equals(word, pos, *forms))
		forms++;

	if (*forms)
		add_replace_ending(word, *uforms, pos);
	else if (!*forms || conv_type == itou) // Verb could always be a る-verb, e.g. 生きます
		add_replace_ending(word, "る", pos + 1); // る-verb

	return 1;
}

/* const char* */
/* check_itou(unistr *word) */
/* { */
/* 	// This will add る to every word */
/* 	return itou_atou_form(word, 1, itou); */
/* } */

void
kanjify(unistr* word)
{
	/* if (startswith(word, "ご") || startswith(word, "お")) */
	/* 	add_replace(word, "御", word->len, 1); */

	if (endswith(word, "ない"))
		add_replace_ending(word, "無い", 2);
	if (endswith(word, "なし"))
		add_replace_ending(word, "無し", 2);
}

int
check_te(unistr* word) /* Use past deinflection instead? */
{
	/* exceptions */
	if (equals(word, "して"))
		return add_str("する");
	else if (equals(word, "きて") || equals(word, "来て"))
		return add_str("来る");
	else if (equals(word, "いって") || equals(word, "行って"))
		return add_str("行く");
	/* ----------- */

	else if (endswith(word, "して"))
		return add_replace_ending(word, "す", 2);

	else if (endswith(word, "いて"))
		return add_replace_ending(word, "く", 2);

	else if (endswith(word, "いで"))
		return add_replace_ending(word, "ぐ", 2);
	else if (endswith(word, "んて"))
	{
		add_replace_ending(word, "む", 2);
		add_replace_ending(word, "ぶ", 2);
		add_replace_ending(word, "ぬ", 2);
		return 1;
	}
	else if (endswith(word, "って"))
	{
		add_replace_ending(word, "る", 2);
		add_replace_ending(word, "う", 2);
		add_replace_ending(word, "つ", 2);
		return 1;
	}
	else if (endswith(word, "て"))
		return add_replace_ending(word, "る", 1); // る-verb

	return 0;
}

int
check_past(unistr* word)
{
	/* exceptions */
	if (equals(word, "した"))
		return add_str("する");
	else if (equals(word, "きた") || equals(word, "来た"))
		return add_str("来る");
	else if (equals(word, "いった") || equals(word, "行った"))
		return add_str("行く");
	/* ----------- */

	else if (endswith(word, "した"))
		return add_replace_ending(word, "す", 2);
	else if (endswith(word, "いた"))
		return add_replace_ending(word, "く", 2);
	else if (endswith(word, "いだ"))
		return add_replace_ending(word, "ぐ", 2);
	else if (endswith(word, "んだ"))
	{
		add_replace_ending(word, "む", 2);
		add_replace_ending(word, "ぶ", 2);
		add_replace_ending(word, "ぬ", 2);
		return 1;
	}
	else if (endswith(word, "った"))
	{
		add_replace_ending(word, "る", 2);
		add_replace_ending(word, "う", 2);
		add_replace_ending(word, "つ", 2);
		return 1;
	}
	else if (endswith(word, "た"))
		return add_replace_ending(word, "る", 1); // る-verb

	return 0;
}

int
check_masu(unistr* word)
{
	return endswith(word, "ます") ? itou_atou_form(word, 3, itou)
	     : endswith(word, "ません") ? itou_atou_form(word, 4, itou)
	     : 0;
}

int
check_shimau(unistr* word)
{
	return endswith(word, "しまう") ? add_truncate(word, 3)
	     : 0;
}

int
check_passive_causative(unistr* word)
{
	return endswith(word, "られる") ? add_replace_ending(word, "る", 3)
	     : endswith(word, "させる") ? add_replace_ending(word, "る", 3)
	     : endswith(word, "れる")   ? itou_atou_form(word, 3, atou)
	     : endswith(word, "せる")   ? itou_atou_form(word, 3, atou)
	     : 0;
}

int
check_adjective(unistr* word)
{
	return endswith(word, "かった") ? add_replace_ending(word, "い", 3)
	     : endswith(word, "くて")   ? add_replace_ending(word, "い", 1)
	     : endswith(word, "よくて") ? add_replace_ending(word, "いい", 3) // e.g. 格好よくて
	     : endswith(word, "さ")     ? add_replace_ending(word, "い", 1)
	     : 0;
}

int
check_volitional(unistr* word)
{
	return endswith(word, "たい") ? itou_atou_form(word, 3, itou)
	     : 0;
}

int
check_negation(unistr* word)
{
	return endswith(word, "ない") ? itou_atou_form(word, 3, atou)
	     : 0;
}

void
deinflect_one_iter(const char *word)
{
	unistr *uniword = init_unistr(word);

	if (uniword->len < 2) return;

	kanjify(uniword);

	check_shimau(uniword);
	check_masu(uniword);
	check_passive_causative(uniword);
	check_volitional(uniword);
	check_negation(uniword);
	check_adjective(uniword);
	check_te(uniword);
	check_past(uniword);

	unistr_free(uniword);
}

GPtrArray *
deinflect(const char *word)
{
	deinfs = g_ptr_array_new_with_free_func(g_free);

	deinflect_one_iter(word);

	for (int i = 0; i < deinfs->len; i++)
		deinflect_one_iter(g_ptr_array_index(deinfs, i));

	return deinfs;
}
