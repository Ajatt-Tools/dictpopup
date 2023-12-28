#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <ankiconnectc.h>

#include "xlib.h"
#include "util.h"
#include "deinflector.h"
#include "structs.h"
#include "popup.h"
#include "readsettings.h"

#include <pthread.h>

#define SDCV_CMD "sdcv -nej --utf8-output"

settings *user_settings;

typedef struct {
	char *p[NUMBER_POSS_ENTRIES];
	GPtrArray *dict;
	char *argv1;
} ThreadData;

void
dictentry_free(void *ptr)
{
	dictentry *de = ptr;
	g_free(de->dictname);
	g_free(de->word);
	g_free(de->definition);
	g_free(de->lookup);
	free(de);
}

/*
   Returns a dictionary entry with the given entries.
   Result needs to be freed with dictentry_free
 */
dictentry *
dictentry_init(char *dictname, char *dictword, char *definition, char *lookup)
{
	dictentry *de = malloc(sizeof(dictentry));
	Stopif(!de, return NULL, "ERROR: Could not allocate memory for dictionary entry.");

	de->dictname = dictname ? g_strdup(dictname) : NULL;
	de->word = dictword ? g_strdup(dictword) : NULL;
	de->definition = definition ? g_strdup(definition) : NULL;
	de->lookup = lookup ? g_strdup(lookup) : NULL;

	return de;
}

/**
   Execute sdcv to look up the word.
   Returns the output as a string in json format, which needs to be freed.
 */
char *
lookup(char *word)
{
	char *sdcv_json = read_cmd_sync("sdcv -nej --utf8-output '%s'", word);
	Stopif(!sdcv_json, exit(1), "Error calling sdcv. Is it installed?");
	return sdcv_json;
}

/*
   Creates a dictionary entry and adds it to dict from given entries.
 */
void
add_to_dictionary(GPtrArray *dict, char *dictname, char *dictword, char *definition, char *lookup)
{
	dictentry *de = dictentry_init(dictname, dictword, definition, lookup);
	g_ptr_array_add(dict, de);
}

int
is_empty_json(char *json)
{
	return !json || !json[0] ? 1 : !strncmp(json, "[]", 2);
}

/*
 * Adds all dictionary entries from json string to dict.
 * Changes json string in process
 * json can be NULL.
 */
void
add_from_json(GPtrArray *dict, char *lookupstr, char *json)
{
	if (is_empty_json(json))
		return;

	str_repl_by_char(json, "\\n", '\n');
	const char *keywords[3] = { "\"dict\"", "\"word\"", "\"definition\"" };
	char *entries[3] = { NULL, NULL, NULL };

	char *start;
	for (char* i = json + 1; *i ; i++)
	{
		for (int k = 0; k < 3; k++)
		{
			if (strncmp(i, keywords[k], strlen(keywords[k])) == 0)
			{       /* TODO: string bounds checking */
				i += strlen(keywords[k]);
				while (*i != '"')
					i++;

				while (*(++i) == '\n') // Skip leading newlines
					;
				start = i;

				while (*i != '"' || *(i - 1) == '\\') // not escaped
					i++;

				*i++ = '\0';

				if (entries[k])
					fprintf(stderr, "WARNING: Overwriting previous entry. \
					    Expects to see dict, word and definition before next entry.");
				entries[k] = start;

				if (entries[0] && entries[1] && entries[2])
				{
					add_to_dictionary(dict, entries[0], entries[1], entries[2], lookupstr);
					for (int l = 0; l < 3; l++)
						entries[l] = NULL;
				}
				break;
			}
		}
	}
}

void
add_word_to_dict(GPtrArray *dict, char *word)
{
	char *sdcv_output = lookup(word);
	add_from_json(dict, word, sdcv_output);
	free(sdcv_output);
}

void
add_deinflections_to_dict(GPtrArray *dict, char *word)
{
	char **deinflections = deinflect(word);

	for (char **ptr = deinflections; *ptr; ptr++)
		add_word_to_dict(dict, *ptr);

	g_strfreev(deinflections);
}

void
create_dictionary_from(GPtrArray *dict, char *word)
{
	add_word_to_dict(dict, word);
	add_deinflections_to_dict(dict, word);
}

char *
boldWord(char *sent, char *word)
{
	gchar   *bdword = g_strdup_printf("<b>%s</b>", word);
	GString *bdsent = g_string_new(sent);

	g_string_replace(bdsent, word, bdword, 0);

	g_free(bdword);
	return g_string_free_and_steal(bdsent);
}

char *
create_furigana(char *kanji, char* reading)
{
	// Broken if kanji contains hiragana
	char *furigana;
	asprintf(&furigana, "%s[%s]", kanji, reading);
	return furigana;
}

void
populate_entries(char *pe[], dictentry* de)
{
	if (user_settings->copysentence)
	{
		notify("Please select the sentence.");
		clipnotify();
		pe[CopiedSentence] = sselp();
		if (user_settings->nukewhitespace)
			nuke_whitespace(pe[CopiedSentence]);
	}

	pe[BoldSentence] = boldWord(pe[CopiedSentence], pe[LookedUpString]);

	pe[DeinflectedLookup] = strdup(de->lookup);

	pe[DictionaryReading] = extract_reading(de->word);

	pe[DictionaryKanji] = extract_kanji(de->word);

	pe[DeinflectedFurigana] = create_furigana(pe[DeinflectedLookup], pe[DictionaryReading]);
}

void
prepare_anki_card(ankicard *ac, char **pe)
{
	ac->deck = user_settings->deck;
	ac->notetype = user_settings->notetype;

	ac->num_fields = user_settings->num_fields;
	ac->fieldnames = user_settings->fieldnames;
	ac->fieldentries = calloc(ac->num_fields, sizeof(char *));

	for (int i = 0; i < ac->num_fields; i++)
		ac->fieldentries[i] = pe[user_settings->fieldmapping[i]];
	// Be careful to not free pe before using the anki card
}

void
p_free(char **p)
{
	for (int i = 0; i < NUMBER_POSS_ENTRIES; i++)
		free(p[i]);
}

void addNote_response(const char *err)
{
	if (!err)
		notify("Sucessfully added card.");
	else
		notify("Error creating card: %s", err);
}

void*
lookup_and_create_dictionary(void *voidin)
{
	ThreadData *in = voidin;
	char **p = in->p;
	GPtrArray *dict = in->dict;
	char *argv1 = in->argv1;

	user_settings = read_user_settings();

	p[LookedUpString] = argv1 ? g_strdup(argv1) : sselp(); //strdup, so can be freed afterwards
	Stopif(!p[LookedUpString] || !*p[LookedUpString], exit(1), "No selection and no argument provided. Exiting.");

	if (user_settings->nukewhitespace)
		nuke_whitespace(p[LookedUpString]);

	p[FocusedWindowName] = getwindowname();

	create_dictionary_from(dict, p[LookedUpString]);
	Stopif(dict->len == 0, p_free(p); g_ptr_array_free(dict, TRUE); exit(1), "No dictionary entry found.");

	dictionary_data_done(user_settings);
	return NULL;
}

void*
display_popup(void *voidin)
{
	ThreadData *in = voidin;
	char **p = in->p;
	GPtrArray *dict = in->dict;

	size_t de_num;
	if (popup(dict, &p[DictionaryDefinition], &de_num, user_settings) && user_settings->ankisupport)
	{
		dictentry *chosen_dict = g_ptr_array_index(dict, de_num);
		populate_entries(p, chosen_dict);

		ankicard ac;
		prepare_anki_card(&ac, p);

		const char *err = ac_addNote(ac, addNote_response);
		if (err) notify("Error communicating with AnkiConnect. Is Anki running?");

		free(ac.fieldentries);
	}

	return NULL;
}

int
main(int argc, char**argv)
{
	ThreadData data;
	for (int i = 0; i < NUMBER_POSS_ENTRIES; i++) data.p[i] = NULL;
	data.dict = g_ptr_array_new_with_free_func(dictentry_free);
	data.argv1 = argc > 1 ? argv[1] : NULL;

	pthread_t threads[2];
	pthread_create(&threads[0], NULL, lookup_and_create_dictionary, &data);
	pthread_create(&threads[1], NULL, display_popup, &data);
	for (int i = 0; i < 2; i++)
		pthread_join(threads[i], NULL);

	p_free(data.p);
	g_ptr_array_free(data.dict, TRUE);
}
