#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <glib.h>
#include <mecab.h>

#include "util.h"

static GPtrArray *deinfs;

#define startswith(str, prefix)                                                           \
	(str.len >= lengthof(prefix) && !u8compare(str.s, (u8*)prefix, lengthof(prefix)))

#define endswith(str, suffix)                                                                                     \
	(str.len >= lengthof(suffix) && !u8compare(str.s + str.len - lengthof(suffix), (u8*)suffix, lengthof(suffix)))

#define IF_STARTSWITH_REPLACE(prefix, replacement)                                 \
	if (startswith(word, prefix))                                              \
	{                                                                          \
		add_replace_prefix(word, s8(replacement), lengthof(replacement));  \
	}

// TODO: Would be cool if the for loop could expand at compile time, so s8(..) can be used
#define IF_ENDSWITH_REPLACE(ending, ...)                                                      \
	if (endswith(word, ending))                                                           \
	{                                                                                     \
		for (char **iterator = (char*[]){ __VA_ARGS__, NULL }; *iterator; iterator++) \
		{                                                                             \
			add_replace_suffix(word, s8fromcstr(*iterator), lengthof(ending));        \
		}                                                                             \
	}

#define IF_ENDSWITH_CONVERT_ATOU(ending)           \
	if (endswith(word, ending))                \
	{                                          \
		atou_form(word, lengthof(ending)); \
	}

#define IF_ENDSWITH_CONVERT_ITOU(ending)           \
	if (endswith(word, ending))                \
	{                                          \
		itou_form(word, lengthof(ending)); \
	}

#define IF_EQUALS_ADD(str, wordtoadd)                    \
	if (s8equals(word, s8(str)))                     \
	{                                   		 \
		g_ptr_array_add(deinfs, s8dup(s8(str))); \
	}


// TODO: Write this properly
static char*
kata2hira(s8 katakana_in)
{
	char* hiragana_out = g_strndup((char*)katakana_in.s, katakana_in.len);
	u8* h = (u8*)hiragana_out;

	while (*h)
	{
		/* Check that this is within the katakana block from E3 82 A0 to E3 83 BF. */
		if (h[0] == 0xe3 && (h[1] == 0x82 || h[1] == 0x83) && h[2] != '\0')
		{
			/* Check that this is within the range of katakana which
			   can be converted into hiragana. */
			if ((h[1] == 0x82 && h[2] >= 0xa1) ||
			    (h[1] == 0x83 && h[2] <= 0xb6) ||
			    (h[1] == 0x83 && (h[2] == 0xbd || h[2] == 0xbe)))
			{
				/* Byte conversion from katakana to hiragana. */
				if (h[2] >= 0xa0)
				{
					h[1] -= 1;
					h[2] -= 0x20;
				}
				else
				{
					h[1] = h[1] - 2;
					h[2] += 0x20;
				}
			}
			h = (unsigned char*)g_utf8_next_char(h);
		}
		else
			h++;
	}

	return hiragana_out;
}

char*
kanji2hira(s8 input)
{
	// TODO: Write this properly
	GString* output = g_string_sized_new(input.len + 2);

	mecab_t *mecab = mecab_new2("");
	const mecab_node_t *node = mecab_sparse_tonode(mecab, (char*)input.s);

	char kata_reading[64];

	for (; node; node = node->next)
	{
		if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE)
			continue;

		sscanf(node->feature, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%[^,],", kata_reading);
		if (strlen(kata_reading) == 0)
		{
			strcpy(kata_reading, node->surface);
			kata_reading[node->length] = '\0';
		}
		char* reading_hira = kata2hira(s8fromcstr(kata_reading));
		g_string_append(output, reading_hira);
		free(reading_hira);
	}

	mecab_destroy(mecab);
	return g_string_free_and_steal(output);
}

/**
 * add_replace_ending:
 * @word: The word whose ending should be replaced
 * @replacement: The UTF-8 encoded string the ending should be replaced with
 * @suffix_len: The length of the suffix (in bytes) that should be replaced
 *
 * Replaces the last @suffix_len bytes of @word with @replacement.
 *
 * Returns: The newly allocated string with replaced ending.
 */
static void
add_replace_suffix(s8 word, s8 replacement, ptrdiff_t suffix_len)
{
	assert(word.s);
	assert(word.len >= suffix_len);
	assert(replacement.len >= 0);

	s8* replstr = g_new(s8, 1);
	replstr->len = word.len - suffix_len + replacement.len;
	replstr->s = g_malloc((gsize)replstr->len);

	memcpy(replstr->s, word.s, (size_t)(word.len - suffix_len));
	if (replacement.len)
		memcpy(replstr->s + word.len - suffix_len, replacement.s, (size_t)replacement.len);

	// Alt:
	/* char* replstr = g_strdup_printf("%.*s%.*s", word.len - suffix_len, word.s, replacement.len, replacement.s); */
	g_ptr_array_add(deinfs, replstr);
}

static void
add_replace_prefix(s8 word, s8 replacement, ptrdiff_t prefix_len)
{
	assert(word.s);
	assert(word.len >= prefix_len);
	assert(replacement.len >= 0);

	s8* replstr = g_new(s8, 1);
	replstr->len = word.len - prefix_len + replacement.len;
	replstr->s = g_malloc((gsize)replstr->len);

	if (replacement.len)
	      memcpy(replstr->s, replacement.s, (size_t)replacement.len);
	memcpy(replstr->s + replacement.len, word.s + prefix_len, (size_t)(word.len - prefix_len));

	// Alt:
	/* char* replstr = g_strdup_printf("%.*s%.*s", replacement.len, replacement.s, word.len - prefix_len, word.s + prefix_len); */
	g_ptr_array_add(deinfs, replstr);
}

/*
 * @word: The word to be converted
 * @len: The length of the ending to be disregarded
 *
 * Converts a word in あ-form to the う-form.
 *
 * Returns: TRUE if any conversion happened, FALSE otherwise
 */
static void
atou_form(s8 word, ptrdiff_t len_ending)
{
	IF_ENDSWITH_REPLACE("", "る");

	word.len -= len_ending;

	IF_ENDSWITH_REPLACE("さ", "す");
	IF_ENDSWITH_REPLACE("か", "く");
	IF_ENDSWITH_REPLACE("が", "ぐ");
	IF_ENDSWITH_REPLACE("ば", "ぶ");
	IF_ENDSWITH_REPLACE("た", "つ");
	IF_ENDSWITH_REPLACE("ま", "む");
	IF_ENDSWITH_REPLACE("わ", "う");
	IF_ENDSWITH_REPLACE("な", "ぬ");
	IF_ENDSWITH_REPLACE("ら", "る");

	word.len += len_ending;
}

/*
 * @word: The word to be converted
 * @len: The length of the ending to be disregarded
 *
 * Converts a word in い-form to the う-form.
 *
 * Returns: TRUE if any conversion happened, FALSE otherwise
 */
static void
itou_form(s8 word, ptrdiff_t len_ending)
{
	/* Word can alway be a る-verb, e.g. 生きます */
	IF_ENDSWITH_REPLACE("", "る");

	word.len -= len_ending;

	IF_ENDSWITH_REPLACE("し", "す");
	IF_ENDSWITH_REPLACE("き", "く");
	IF_ENDSWITH_REPLACE("ぎ", "ぐ");
	IF_ENDSWITH_REPLACE("び", "ぶ");
	IF_ENDSWITH_REPLACE("ち", "つ");
	IF_ENDSWITH_REPLACE("み", "む");
	IF_ENDSWITH_REPLACE("い", "う");
	IF_ENDSWITH_REPLACE("に", "ぬ");
	IF_ENDSWITH_REPLACE("り", "る");

	word.len += len_ending;
}

static void
kanjify(s8 word)
{
	IF_STARTSWITH_REPLACE("ご", "御");
	IF_STARTSWITH_REPLACE("お", "御");

	IF_ENDSWITH_REPLACE("ない", "無い");
	IF_ENDSWITH_REPLACE("なし", "無し");
	IF_ENDSWITH_REPLACE("つく", "付く");
}

static void
check_te(s8 word)
{
	/* exceptions */
	IF_EQUALS_ADD("きて", "来る");
	IF_ENDSWITH_REPLACE("来て", "来る");
	IF_EQUALS_ADD("いって", "行く");
	IF_ENDSWITH_REPLACE("行って", "行く");
	/* ----------- */

	IF_ENDSWITH_REPLACE("して", "する", "す");
	IF_ENDSWITH_REPLACE("いて", "く");
	IF_ENDSWITH_REPLACE("いで", "ぐ");
	IF_ENDSWITH_REPLACE("んで", "む", "ぶ", "ぬ");
	IF_ENDSWITH_REPLACE("って", "る", "う", "つ");
	IF_ENDSWITH_REPLACE("て", "る");
}

static void
check_past(s8 word)
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
}

static void
check_masu(s8 word)
{
	IF_ENDSWITH_CONVERT_ITOU("ます");
	IF_ENDSWITH_CONVERT_ITOU("ません");
}

static void
check_shimau(s8 word)
{
	IF_ENDSWITH_REPLACE("しまう", "");
	IF_ENDSWITH_REPLACE("ちゃう", "る");
	IF_ENDSWITH_REPLACE("いじゃう", "ぐ");
	IF_ENDSWITH_REPLACE("いちゃう", "く");
	IF_ENDSWITH_REPLACE("しちゃう", "す");
}

static void
check_passive_causative(s8 word)
{
	IF_ENDSWITH_REPLACE("られる", "る");
	IF_ENDSWITH_REPLACE("させる", "る");
	IF_ENDSWITH_CONVERT_ATOU("れる");
	IF_ENDSWITH_CONVERT_ATOU("せる");
}

static void
check_adjective(s8 word)
{
	IF_ENDSWITH_REPLACE("よくて", "いい");
	IF_ENDSWITH_REPLACE("かった", "い");
	IF_ENDSWITH_REPLACE("くない", "い");
	IF_ENDSWITH_REPLACE("くて", "い");
	IF_ENDSWITH_REPLACE("そう", "い");
	IF_ENDSWITH_REPLACE("さ", "い");
	IF_ENDSWITH_REPLACE("げ", "い");
	IF_ENDSWITH_REPLACE("しく", "しい"); //FIXME
}

static void
check_volitional(s8 word)
{
	IF_ENDSWITH_CONVERT_ITOU("たい");
}

static void
check_negation(s8 word)
{
	IF_ENDSWITH_CONVERT_ATOU("ない");
	IF_ENDSWITH_CONVERT_ATOU("ねぇ");
}

static void
check_potential(s8 word)
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
}

static void
check_conditional(s8 word)
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
}

static void
deinflect_one_iter(s8 word)
{
	check_shimau(word);
	check_adjective(word);
	check_masu(word);
	check_passive_causative(word);
	check_volitional(word);
	check_negation(word);
	check_te(word);
	check_past(word);
	check_potential(word);
	check_conditional(word);
	kanjify(word);
}

s8**
deinflect(s8 word)
{
	deinfs = g_ptr_array_new_with_free_func(g_free);

	deinflect_one_iter(word);
	for (u32 i = 0; i < deinfs->len; i++)
	{
		s8* entry = g_ptr_array_index(deinfs, i);
		deinflect_one_iter(*entry);
	}

	itou_form(word, 0);

	g_ptr_array_add(deinfs, NULL);  /* Add NULL terminator */
	return (s8**)g_ptr_array_steal(deinfs, NULL);
}
