#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <ankiconnectc.h>
#include <pthread.h>

#include "xlib.h"
#include "util.h"
#include "deinflector.h"
#include "structs.h"
#include "popup.h"
#include "readsettings.h"

enum PossibleEntries {
	Empty,                  /* An empty string. */
	LookedUpString,         /* The word this program was called with */
	DeinflectedLookup,      /* The deinflected version of the lookup string */
	CopiedSentence,         /* The copied sentence */
	BoldSentence,           /* The copied sentence with lookup string in bold */
	DictionaryKanji,        /* All kanji writings from dictionary entry */
	DictionaryReading,      /* The hiragana reading form the dictionary entry */
	DictionaryDefinition,   /* The chosen dictionary definition */
	DeinflectedFurigana,    /* The string: [DeinflectedLookup][DictionaryReading] */
	FocusedWindowName       /* The name of the focused window at lookup time */
};
typedef struct possible_entries_s {
	char *empty;
	char *lookup;
	char *deinflected_lookup;
	char *copied_sentence;
	char *bold_sentence;
	char *dictionary_kanji;
	char *dictionary_reading;
	char *dictionary_definition;
	char *deinflected_furigana;
	char *windowname;
} possible_entries_s;

typedef struct {
	char *p[NUMBER_POSS_ENTRIES];
	GPtrArray *dict;
	char *argv1;
} ThreadData;

/* GLOBALS */
char *SDCV_PATH;
/* ------- */

/* --- dictentry --- */
/*
 * Returns a dictionary entry with the given entries.
 * Result needs to be freed with dictentry_free
 */
dictentry *
dictentry_new(char *dictname, char *dictword, char *definition, char *lookup)
{
	dictentry *de = malloc(sizeof(dictentry));
	Stopif(!de, return NULL, "ERROR: Could not allocate memory for dictionary entry.");

	*de = (dictentry) {
		.dictname = dictname ? g_strdup(dictname) : NULL,
		.word = dictword ? g_strdup(dictword) : NULL,
		.definition = definition ? g_strdup(definition) : NULL,
		.lookup = lookup ? g_strdup(lookup) : NULL
	};

	return de;
}

/*
 * Frees a dictentry created with dictentry_new
 */
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
/* -------------- */

void
retrieve_sdcv_path()
{
	SDCV_PATH = g_find_program_in_path("sdcv");
	Stopif(!SDCV_PATH, exit(1), "Could not find sdcv executable. Is it installed?");
}

/*
 * Execute sdcv to look up the word.
 * Returns: The output as a string in json format. Needs to be freed.
 */
char *
lookup(char *word)
{
	char *sdcv_json = read_cmd_sync((char *[]) { SDCV_PATH, "-nej", "--utf8-output", word, NULL });
	Stopif(!sdcv_json, exit(1), "Error executing sdcv.");
	return sdcv_json;
}

/*
 * Creates a dictionary entry and adds it to dict from given entries.
 */
void
add_to_dictionary(GPtrArray *dict, char *dictname, char *dictword, char *definition, char *lookup)
{
	dictentry *de = dictentry_new(dictname, dictword, definition, lookup);
	g_ptr_array_add(dict, de);
}

int
is_empty_json(char *json)
{
	return !json || !*json ? 1 : (strncmp(json, "[]", 2) == 0);
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
	const char *keywords[] = { "\"dict\"", "\"word\"", "\"definition\"" };
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
					g_warning("WARNING: Overwriting previous entry. \
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
	/* add_deinflections_to_dict(dict, word); */
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
	if (cfg.copysentence)
	{
		notify(0, "Please select the sentence.");
		clipnotify();
		pe[CopiedSentence] = sselp();
		if (cfg.nukewhitespace)
			nuke_whitespace(pe[CopiedSentence]);
	}

	pe[BoldSentence] = boldWord(pe[CopiedSentence], pe[LookedUpString]);

	pe[DeinflectedLookup] = strdup(de->lookup);

	pe[DictionaryReading] = extract_reading(de->word);

	pe[DictionaryKanji] = extract_kanji(de->word);

	pe[DeinflectedFurigana] = create_furigana(pe[DeinflectedLookup], pe[DictionaryReading]);
}

void
check_addNote_response(const char *err)
{
	if (err)
		notify(1, "Error creating card: %s", err);
	else
		notify(0, "Sucessfully added card.");
}

const char*
send_ankicard(char **pe)
{
	ankicard *ac = ankicard_new();
	*ac = (ankicard) {
		.deck = cfg.deck,
		.notetype = cfg.notetype,
		.num_fields = cfg.num_fields,
		.fieldnames = cfg.fieldnames,
		.fieldentries = calloc(cfg.num_fields, sizeof(char *))
	};

	for (int i = 0; i < ac->num_fields; i++)
		ac->fieldentries[i] = pe[cfg.fieldmapping[i]];

	const char *err = ac_addNote(ac, check_addNote_response);

	free(ac->fieldentries);
	free(ac);
	return err;
}

void
p_free(char **p)
{
	for (int i = 0; i < NUMBER_POSS_ENTRIES; i++)
		free(p[i]);
}

void*
lookup_and_create_dictionary(void *voidin)
{
	ThreadData *in = voidin;
	char **p = in->p;
	GPtrArray *dict = in->dict;
	char *argv1 = in->argv1;

	p[LookedUpString] = argv1 ? g_strdup(argv1) : sselp(); //strdup, so can be freed afterwards
	Stopif(!p[LookedUpString] || !*p[LookedUpString], exit(1), "No selection and no argument provided. Exiting.");

	if (cfg.nukewhitespace)
		nuke_whitespace(p[LookedUpString]);

	p[FocusedWindowName] = getwindowname();

	create_dictionary_from(dict, p[LookedUpString]);

	Stopif(dict->len == 0, p_free(p); g_ptr_array_free(dict, TRUE); exit(1), "No dictionary entry found.");

	dictionary_data_done();
	return NULL;
}

void*
display_popup(void *voidin)
{
	ThreadData *in = voidin;
	char **p = in->p;
	GPtrArray *dict = in->dict;

	size_t de_num;
	if (popup(dict, &p[DictionaryDefinition], &de_num) && cfg.ankisupport)
	{
		dictentry *chosen_dict = g_ptr_array_index(dict, de_num);
		populate_entries(p, chosen_dict);

		const char *err = send_ankicard(p);
		if (err)
			notify(1, "%s", err);
	}

	return NULL;
}

int
main(int argc, char**argv)
{
	read_user_settings();
	retrieve_sdcv_path();

	ThreadData data;
	for (int i = 0; i < NUMBER_POSS_ENTRIES; i++)
		data.p[i] = NULL;
	data.dict = g_ptr_array_new_with_free_func(dictentry_free);
	data.argv1 = argc > 1 ? argv[1] : NULL;

	pthread_t threads[2];
	pthread_create(&threads[0], NULL, lookup_and_create_dictionary, &data);
	pthread_create(&threads[1], NULL, display_popup, &data);
	for (int i = 0; i < 2; i++)
		pthread_join(threads[i], NULL);

	p_free(data.p);
	g_ptr_array_free(data.dict, TRUE);
	free(SDCV_PATH);
	settings_free();
}
