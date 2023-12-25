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
add_replace_ending(unistr* word, const char *c, size_t len)
{
	char *replaced_str = unistr_replace_ending(word, c, len);

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
itou_atou_form(unistr *word, size_t len_ending, int conv_type)
{
	Stopif(len_ending > word->len - 3, return 0, "ERROR: Received an invalid position in itou_atou_form function.");

	const char *aforms[] = { "さ", "か", "が", "ば", "た", "ま", "わ", "な", "ら", NULL };
	const char *iforms[] = { "し", "き", "ぎ", "び", "ち", "み", "い", "に", "り", NULL };
	const char *uforms[] = { "す", "く", "ぐ", "ぶ", "つ", "む", "う", "ぬ", "る", NULL };
	const char **forms = (conv_type == atou) ? aforms : iforms;

	char *chr = get_ptr_to_char_before(word, len_ending);
	while (*forms && strncmp(chr, *forms, strlen(*forms)))
		forms++;

	if (*forms)
		add_replace_ending(word, *uforms, len_ending + strlen(*forms));
	else if (!*forms || conv_type == itou) // Verb could always be a る-verb, e.g. 生きます
		add_replace_ending(word, "る", len_ending); // る-verb

	return 1;
}

/* const char* */
/* check_itou(unistr *word) */
/* { */
/* 	// This will add る to every word */
/* 	return itou_atou_form(word, 1, itou); */
/* } */

int
kanjify(unistr* word)
{
	/* if (startswith(word, "ご") || startswith(word, "お")) */

	IF_ENDSWITH_REPLACE_1("ない", "無い");
	IF_ENDSWITH_REPLACE_1("なし", "無し");
	return 0;
}

int
check_te(unistr* word) /* Use past deinflection instead? */
{
	/* exceptions */
	IF_EQUALS_ADD("して", "為る");
	IF_EQUALS_ADD("きて", "来る");
	IF_EQUALS_ADD("来て", "来る");
	IF_EQUALS_ADD("いって", "行く");
	IF_EQUALS_ADD("行って", "行く");
	/* ----------- */

	IF_ENDSWITH_REPLACE_1("して", "す");
	IF_ENDSWITH_REPLACE_1("いて", "く");
	IF_ENDSWITH_REPLACE_1("いで", "ぐ");
	IF_ENDSWITH_REPLACE_3("んで", "む", "ぶ", "ぬ");
	IF_ENDSWITH_REPLACE_3("って", "る", "う", "つ");
	IF_ENDSWITH_REPLACE_1("て", "る");

	return 0;
}

int
check_past(unistr* word)
{
	/* exceptions */
	IF_EQUALS_ADD("した", "為る");
	IF_EQUALS_ADD("きた", "来る");
	IF_EQUALS_ADD("来た", "来る");
	IF_EQUALS_ADD("いった", "行く");
	IF_EQUALS_ADD("行った", "行く");
	/* ----------- */

	IF_ENDSWITH_REPLACE_1("した", "す");
	IF_ENDSWITH_REPLACE_1("いた", "く");
	IF_ENDSWITH_REPLACE_1("いだ", "ぐ");
	IF_ENDSWITH_REPLACE_3("んだ", "む", "ぶ", "ぬ");
	IF_ENDSWITH_REPLACE_3("った", "る", "う", "つ");
	IF_ENDSWITH_REPLACE_1("た", "る");

	return 0;
}

int
check_masu(unistr* word)
{
	IF_ENDSWITH_CONVERT_ITOU("ます");
	IF_ENDSWITH_CONVERT_ITOU("ません");

	return 0;
}

int
check_shimau(unistr* word)
{
	IF_ENDSWITH_REPLACE_1("しまう", "");
	IF_ENDSWITH_REPLACE_1("ちゃう", "る");
	IF_ENDSWITH_REPLACE_1("いじゃう", "ぐ");
	IF_ENDSWITH_REPLACE_1("いちゃう", "く");
	IF_ENDSWITH_REPLACE_1("しちゃう", "す");

	return 0;
}

int
check_passive_causative(unistr* word)
{
	IF_ENDSWITH_REPLACE_1("られる", "る");
	IF_ENDSWITH_REPLACE_1("させる", "る");
	IF_ENDSWITH_CONVERT_ATOU("れる");
	IF_ENDSWITH_CONVERT_ATOU("せる");

	return 0;
}

int
check_adjective(unistr* word)
{
	IF_ENDSWITH_REPLACE_1("かった", "い");
	IF_ENDSWITH_REPLACE_1("くない", "い");
	IF_ENDSWITH_REPLACE_1("くて", "い");
	IF_ENDSWITH_REPLACE_1("よくて", "いい");
	IF_ENDSWITH_REPLACE_1("さ", "い");

	return 0;
}

int
check_volitional(unistr* word)
{
	IF_ENDSWITH_CONVERT_ITOU("たい");
	return 0;
}

int
check_negation(unistr* word)
{
	IF_ENDSWITH_CONVERT_ATOU("ない");
	return 0;
}

int check_potential(unistr* word)
{
	/* Exceptions */
	IF_EQUALS_ADD("できる", "為る");
	IF_EQUALS_ADD("こられる", "来る");
	/* ---------- */

	IF_ENDSWITH_REPLACE_1("せる", "す");
	IF_ENDSWITH_REPLACE_1("ける", "く");
	IF_ENDSWITH_REPLACE_1("べる", "ぶ");
	IF_ENDSWITH_REPLACE_1("てる", "つ");
	IF_ENDSWITH_REPLACE_1("める", "む");
	IF_ENDSWITH_REPLACE_1("れる", "る");
	IF_ENDSWITH_REPLACE_1("ねる", "ぬ");
	IF_ENDSWITH_REPLACE_1("える", "う");
	return 0;
}

void
deinflect_one_iter(const char *word)
{
	unistr *uniword = init_unistr(word);

	if(check_shimau(uniword));
	else if(check_adjective(uniword));
	else if(check_masu(uniword));
	else if(check_passive_causative(uniword));
	else if(check_volitional(uniword));
	else if(check_negation(uniword));
	else if(check_te(uniword));
	else if(check_past(uniword));
	else if(check_potential(uniword));
	else kanjify(uniword);

	unistr_free(uniword);
}

char**
deinflect(const char *word)
{
	deinfs = g_ptr_array_new_with_free_func(g_free);

	deinflect_one_iter(word);

	for (int i = 0; i < deinfs->len; i++)
		deinflect_one_iter(g_ptr_array_index(deinfs, i));

	
	g_ptr_array_add (deinfs, NULL); /* Add NULL terminator */
	return (char**) g_ptr_array_steal (deinfs, NULL);
}
