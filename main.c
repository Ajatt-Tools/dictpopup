#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <ankiconnectc.h>

#include "xlib.h"
#include "util.h"
#define IMPORT_VARIABLES
#include "config.h"
#undef IMPORT_VARIABLES
#include "deinflector.h"
#include "structs.h"
#include "popup.h"
/* #include "debug.h" */

#define SDCV_CMD "sdcv -nej --utf8-output"

#define Stopif(assertion, error_action, ...)          \
	if (assertion) {                              \
		fprintf(stderr, __VA_ARGS__);         \
		fprintf(stderr, "\n");                \
		error_action;                         \
	}

size_t num_fields = sizeof(fieldnames) / sizeof(fieldnames[0]);

typedef struct {
	unsigned int copysent : 1; /* Prompt for sentence copy */
	unsigned int nuke_whitespace : 1; /* Remove whitespace */
	unsigned int win_width;
	unsigned int win_height;
	unsigned int win_margin;
} settings;

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
	char *cmd;
	asprintf(&cmd, SDCV_CMD " '%s'", word); // single quot to make sure no cmd gets interpreted

	FILE *fp;
	fp = popen(cmd, "r");
	Stopif(!fp, free(cmd); exit(1), "Failed calling command \"%s\". Is sdcv installed?", cmd);

	char *buf = NULL;
	size_t size = 0;
	Stopif(!getline(&buf, &size, fp), free(cmd); exit(1), "Failed reading output of the command: %s.", cmd);

	pclose(fp);
	free(cmd);
	return buf;
}

int
is_empty_json(char *json)
{
	return !json || !json[0] ? 1 : !strncmp(json, "[]", 2);
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

/*
 * Adds all dictionary entries from json string to dict.
 * Changes json in process
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
					fprintf(stderr, "WARNING: Overwriting previous entry. Expects to see all of dict, word and definition before next entry.");
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

GPtrArray*
create_dictionary_from(char *word)
{
	GPtrArray *dict = g_ptr_array_new_with_free_func(dictentry_free);

	add_word_to_dict(dict, word);
	add_deinflections_to_dict(dict, word);

	return dict;
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
guess_kanji_writing(char **kanji_writings, char* reading, char *lookup)
{
	return strdup(*kanji_writings);
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
	notify("Please select the sentence.");
	clipnotify();
	pe[CopiedSentence] = sselp();
	nuke_whitespace(pe[CopiedSentence]);

	pe[BoldSentence] = boldWord(pe[CopiedSentence], pe[LookedUpString]);

	pe[DeinflectedLookup] = de->lookup;

	pe[DictionaryReading] = extract_reading(de->word);

	pe[DictionaryKanji] = extract_kanji(de->word);

	pe[DeinflectedFurigana] = create_furigana(pe[DeinflectedLookup], pe[DictionaryReading]);
}

void
prepare_anki_card(ankicard *ac, char **pe)
{
	ac->deck = ANKI_DECK;
	ac->notetype = ANKI_MODEL;

	ac->num_fields = num_fields;
	ac->fieldnames = fieldnames;
	ac->fieldentries = calloc(num_fields, sizeof(char *));

	for (int i = 0; i < num_fields; i++)
		ac->fieldentries[i] = pe[fieldmapping[i]];
}

void
p_free(char **p)
{
	for (int i = 0; i < NUMBER_POSS_ENTRIES; i++)
		free(p[i]);
}


int
main(int argc, char**argv)
{
	char *p[NUMBER_POSS_ENTRIES] = { NULL };

	p[LookedUpString] = (argc > 1) ? g_strdup(argv[1]) : sselp(); //strdup, so can be freed afterwards
	Stopif(!p[LookedUpString] || !*p[LookedUpString], return 1, "No selection and no argument provided. Exiting.");

	nuke_whitespace(p[LookedUpString]);

	p[FocusedWindowName] = getwindowname();

	GPtrArray *dict = create_dictionary_from(p[LookedUpString]);
	Stopif(dict->len == 0, p_free(p); g_ptr_array_free(dict, TRUE); return 1, "No dictionary entry found.");

	size_t de_num = 0;
	if (popup(dict, &p[DictionaryDefinition], &de_num))
	{
		dictentry *chosen_dict = g_ptr_array_index(dict, de_num);
		populate_entries(p, chosen_dict);

		ankicard ac;
		prepare_anki_card(&ac, p);

		const char *err = addNote(ac);
		Stopif(err, , "Error creating note: %s", err);

		free(ac.fieldentries);
	}

	p_free(p);
	g_ptr_array_free(dict, TRUE);
}
