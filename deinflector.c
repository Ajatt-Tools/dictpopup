#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include "unistr.h"
#include "kanaconv.h"

GPtrArray *deinfs;

/*
 * Replaces the last @len bytes with @c and adds it to the deinflections array.
 */
void
add_replace_ending(unistr* word, const char *c, size_t len)
{
	char *replaced_str = unistr_replace_ending(word, c, len);
	g_ptr_array_add(deinfs, replaced_str);
}

/*
 * Adds @str to the deinflections array.
 */
int
add_str(const char* str)
{
	g_ptr_array_add(deinfs, g_strdup(str));
	return 1;
}

/*
 * @word: The word to be converted
 * @len: The length of the ending to be disregarded
 *
 * Converts a word in あ-form to the う-form.
 *
 * Returns: TRUE if any conversion happened, FALSE otherwise
 */
int
atou_form(unistr* word, size_t len_ending)
{
	add_replace_ending(word, "る", len_ending);

	word->len -= len_ending;

	IF_ENDSWITH_REPLACE("さ", "す");
	IF_ENDSWITH_REPLACE("か", "く");
	IF_ENDSWITH_REPLACE("が", "ぐ");
	IF_ENDSWITH_REPLACE("ば", "ぶ");
	IF_ENDSWITH_REPLACE("た", "つ");
	IF_ENDSWITH_REPLACE("ま", "む");
	IF_ENDSWITH_REPLACE("わ", "う");
	IF_ENDSWITH_REPLACE("な", "ぬ");
	IF_ENDSWITH_REPLACE("ら", "る");

	word->len += len_ending;

	return 0;
}

/*
 * @word: The word to be converted
 * @len: The length of the ending to be disregarded
 *
 * Converts a word in い-form to the う-form.
 *
 * Returns: TRUE if any conversion happened, FALSE otherwise
 */
int
itou_form(unistr* word, size_t len_ending)
{
	/* Word can alway be a る-verb, e.g. 生きます */
	add_replace_ending(word, "る", len_ending);

	word->len -= len_ending;

	IF_ENDSWITH_REPLACE("し", "す");
	IF_ENDSWITH_REPLACE("き", "く");
	IF_ENDSWITH_REPLACE("ぎ", "ぐ");
	IF_ENDSWITH_REPLACE("び", "ぶ");
	IF_ENDSWITH_REPLACE("ち", "つ");
	IF_ENDSWITH_REPLACE("み", "む");
	IF_ENDSWITH_REPLACE("い", "う");
	IF_ENDSWITH_REPLACE("に", "ぬ");
	IF_ENDSWITH_REPLACE("り", "る");

	word->len += len_ending;

	return 0;
}

int
kanjify(unistr* word)
{
	if (startswith(word, "ご") || startswith(word, "お"))
	{       // FIXME: Cleaner implementation
		g_autofree gchar* word_copy = g_strdup(word->str);
		memcpy(word_copy, "御", strlen("ご"));
		add_str(word_copy);
	}

	IF_ENDSWITH_REPLACE("ない", "無い");
	IF_ENDSWITH_REPLACE("なし", "無し");
	IF_ENDSWITH_REPLACE("つく", "付く");

	return 0;
}

int
check_te(unistr* word)
{
	/* exceptions */
	IF_EQUALS_ADD("きて", "来る");
	IF_EQUALS_ADD("来て", "来る");
	IF_EQUALS_ADD("いって", "行く");
	IF_ENDSWITH_REPLACE("行って", "行く");
	/* ----------- */

	IF_ENDSWITH_REPLACE("して", "する", "す");
	IF_ENDSWITH_REPLACE("いて", "く");
	IF_ENDSWITH_REPLACE("いで", "ぐ");
	IF_ENDSWITH_REPLACE("んで", "む", "ぶ", "ぬ");
	IF_ENDSWITH_REPLACE("って", "る", "う", "つ");
	IF_ENDSWITH_REPLACE("て", "る");

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
	IF_ENDSWITH_REPLACE("行った", "行く");
	/* ----------- */

	IF_ENDSWITH_REPLACE("した", "す");
	IF_ENDSWITH_REPLACE("いた", "く");
	IF_ENDSWITH_REPLACE("いだ", "ぐ");
	IF_ENDSWITH_REPLACE("んだ", "む", "ぶ", "ぬ");
	IF_ENDSWITH_REPLACE("った", "る", "う", "つ");
	IF_ENDSWITH_REPLACE("た", "る");

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
	IF_ENDSWITH_REPLACE("しまう", "");
	IF_ENDSWITH_REPLACE("ちゃう", "る");
	IF_ENDSWITH_REPLACE("いじゃう", "ぐ");
	IF_ENDSWITH_REPLACE("いちゃう", "く");
	IF_ENDSWITH_REPLACE("しちゃう", "す");

	return 0;
}

int
check_passive_causative(unistr* word)
{
	IF_ENDSWITH_REPLACE("られる", "る");
	IF_ENDSWITH_REPLACE("させる", "る");
	IF_ENDSWITH_CONVERT_ATOU("れる");
	IF_ENDSWITH_CONVERT_ATOU("せる");

	return 0;
}

int
check_adjective(unistr* word)
{
	IF_ENDSWITH_REPLACE("よくて", "いい");
	IF_ENDSWITH_REPLACE("かった", "い");
	IF_ENDSWITH_REPLACE("くない", "い");
	IF_ENDSWITH_REPLACE("くて", "い");
	IF_ENDSWITH_REPLACE("そう", "い");
	IF_ENDSWITH_REPLACE("さ", "い");
	IF_ENDSWITH_REPLACE("げ", "い");
	IF_ENDSWITH_REPLACE("しく", "しい"); //FIXME

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
	IF_ENDSWITH_CONVERT_ATOU("ねぇ");
	return 0;
}

int
check_potential(unistr* word)
{
	/* Exceptions */
	IF_EQUALS_ADD("できる", "為る");
	IF_EQUALS_ADD("こられる", "来る");
	/* ---------- */

	IF_ENDSWITH_REPLACE("せる", "す");
	IF_ENDSWITH_REPLACE("ける", "く");
	IF_ENDSWITH_REPLACE("べる", "ぶ");
	IF_ENDSWITH_REPLACE("てる", "つ");
	IF_ENDSWITH_REPLACE("める", "む");
	IF_ENDSWITH_REPLACE("れる", "る");
	IF_ENDSWITH_REPLACE("ねる", "ぬ");
	IF_ENDSWITH_REPLACE("える", "う");
	return 0;
}

int
check_conditional(unistr* word)
{
	IF_ENDSWITH_REPLACE("せば", "す");
	IF_ENDSWITH_REPLACE("けば", "く");
	IF_ENDSWITH_REPLACE("げば", "ぐ");
	IF_ENDSWITH_REPLACE("べば", "ぶ");
	IF_ENDSWITH_REPLACE("てば", "つ");
	IF_ENDSWITH_REPLACE("めば", "む");
	IF_ENDSWITH_REPLACE("えば", "う");
	IF_ENDSWITH_REPLACE("ねば", "ぬ");
	IF_ENDSWITH_REPLACE("れば", "る");
	return 0;
}

void
deinflect_one_iter(const char *word)
{
	unistr *uniword = unistr_new(word);

	check_shimau(uniword);
	check_adjective(uniword);
	check_masu(uniword);
	check_passive_causative(uniword);
	check_volitional(uniword);
	check_negation(uniword);
	check_te(uniword);
	check_past(uniword);
	check_potential(uniword);
	check_conditional(uniword);
	kanjify(uniword);

	unistr_free(uniword);
}

char**
deinflect(const char *word)
{
	deinfs = g_ptr_array_new_with_free_func(g_free);

	deinflect_one_iter(word);
	for (int i = 0; i < deinfs->len; i++)
		deinflect_one_iter(g_ptr_array_index(deinfs, i));

	unistr *uword = unistr_new(word);
	itou_form(uword, 0);
	unistr_free(uword);

	g_ptr_array_add(deinfs, NULL);  /* Add NULL terminator */
	return (char**)g_ptr_array_steal(deinfs, NULL);
}
