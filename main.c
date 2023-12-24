#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <ankiconnectc.h>

#include "xlib.h"
#include "util.h"
#include "config.h"
#include "deinflector.h"
#include "structs.h"
#include "popup.h"
/* #include "debug.h" */

#define Stopif(assertion, error_action, ...)          \
	if (assertion) {                              \
		fprintf(stderr, __VA_ARGS__);         \
		fprintf(stderr, "\n");                \
		error_action;                         \
	}

#define NUMBER_POSS_ENTRIES 9

enum PossibleEntries {
	Empty,
	LookedUpString,
	CopiedSentence,
	BoldSentence,
	DictionaryKanji,
	DictionaryReading,
	DictionaryFurigana,
	DictionaryDefinition,
	FocusedWindowName
};

char *fieldnames[] = { "SentKanji", "VocabKanji", "VocabFurigana", "VocabDef", "Notes" };
int fieldmapping[] = { BoldSentence, DictionaryKanji, DictionaryFurigana, DictionaryDefinition, FocusedWindowName };
size_t num_fields = sizeof(fieldnames) / sizeof(fieldnames[0]);

typedef struct {
	unsigned int copysent : 1; /* Prompt for sentence copy */
	unsigned int nuke_whitespace : 1; /* Remove whitespace */
	unsigned int win_width;
	unsigned int win_height;
	unsigned int win_margin;
} settings;

/**
   Execute sdcv to look up the word.
   Returns a json string which needs to be freed.
 */
char *
lookup(char *word)
{
	char *cmd;
	//Passing the user input straight in like that might be dangerous.
	asprintf(&cmd, "sdcv -nej --utf8-output \"%s\"", word);

	FILE *fp;
	fp = popen(cmd, "r");
	Stopif(!fp, free(cmd); exit(1), "Failed calling command %s. Is sdcv installed?", cmd);

	char   *buf = NULL;
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
   Expects: e[0] = dictionary name, e[1] = dictionary word, e[2] = dictionary definition
 */
void
add_dictionary_entry(GPtrArray *dict, char **e)
{
	dictentry *de = malloc(sizeof(dictentry));
	Stopif(!de, return , "ERROR: Could not allocate memory for dictionary entry.");

	de->dictname = strdup(e[0]);
	de->word = strdup(e[1]);
	de->definition = strdup(e[2]);
	Stopif(!de->dictname || !de->word || !de->definition,
	       return , "ERROR: Could not allocate memory for dictionary entry.");

	g_ptr_array_add(dict, de);
}

/* 
   Adds all dictionary entries from json string to dict.
   Destroys json string in the process.
*/
void
add_from_json(GPtrArray *dict, char *json)
{
	if (is_empty_json(json))
		return;

	str_repl_by_char(json, "\\n", '\n');
	const char *keywords[3] = { "\"dict\"", "\"word\"", "\"definition\"" };
	char *entries[3] = { NULL, NULL, NULL };

	int start;
	for (int i = 1; json[i] != '\0'; i++)
	{
		for (int k = 0; k < 3; k++)
		{
			if (strncmp(json + i, keywords[k], strlen(keywords[k])) == 0)
			{
				i += strlen(keywords[k]);
				while (json[i] != '"' || json[i - 1] == '\\')
					i++;                         /* TODO: string bounds checking */
				while (json[++i] == '\n') // Skip leading newlines
					;     
				start = i;
				while (json[i] != '"' || json[i - 1] == '\\')
					i++;

				json[i++] = '\0';

				entries[k] = json + start;
			}
			if (entries[0] && entries[1] && entries[2])
			{
				add_dictionary_entry(dict, entries);
				for (int l = 0; l < 3; l++)
					entries[l] = NULL;
			}
		}
	}
}

void
add_word_to_dict(GPtrArray *dict, char *word)
{
	char *sdcv_output = lookup(word);
	add_from_json(dict, sdcv_output);
	free(sdcv_output);
}

void
add_deinflections_to_dict(GPtrArray *dict, char *word)
{
	GPtrArray *deinflections = deinflect(word);
	Stopif(!deinflections, return , "Received a null pointer in add_deinflections_to_dict");

	for (int i = 0; i < deinflections->len; i++)
		add_word_to_dict(dict, g_ptr_array_index(deinflections, i));

	g_ptr_array_free(deinflections, TRUE);
}

void
free_dictentry(void *ptr)
{
	dictentry *de = ptr;
	free(de->dictname);
	free(de->word);
	free(de->definition);
	free(de);
}

GPtrArray*
create_dictionary_from(char *word)
{
	GPtrArray *dict = g_ptr_array_new_with_free_func(free_dictentry);

	add_word_to_dict(dict, word);
	add_deinflections_to_dict(dict, word);

	return dict;
}

void
edit_wname(char *wname)
{
	/* Strips unnecessary stuff from the windowname */
	// TODO: Implement
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

void
add_entry(char *pe[], char *input, int entry_num)
{
	switch (entry_num)
	{
	case LookedUpString: // input = looked up word
		nuke_whitespace(input);
		pe[LookedUpString] = input;
		break;

	case CopiedSentence: // input = looked up word (ignored)
		notify("Please select the sentence.");
		clipnotify();
		nuke_whitespace(pe[CopiedSentence] = sselp());
		pe[BoldSentence] = boldWord(pe[CopiedSentence], input);
		break;

	case DictionaryKanji:
	case DictionaryReading:
	case DictionaryFurigana: // input: dictionary word
		pe[DictionaryKanji] = extract_kanji(input);
		pe[DictionaryReading] = extract_reading(input);
		asprintf(&pe[DictionaryFurigana], "%s[%s]", pe[DictionaryKanji], pe[DictionaryReading]);
		break;

	case DictionaryDefinition: // input: dictionary definition / selection to add
		pe[DictionaryDefinition] = input;
		break;

	case FocusedWindowName: // input = window name
		edit_wname(input);
		pe[FocusedWindowName] = input;
		break;

	default:
		fprintf(stderr, "ERROR: Tried to add unknown entry.");
	}
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
free_pe(char **pe)
{
	for (int i = 0; i < NUMBER_POSS_ENTRIES; i++)
		free(pe[i]);
}


int
main(int argc, char**argv)
{
	char *pe[NUMBER_POSS_ENTRIES] = { NULL };

	char *luw = (argc > 1) ? strdup(argv[1]) : sselp(); //strdup, so can be freed afterwards
	Stopif(!luw || luw[0] == '\0', return 1, "No selection and no argument provided. Exiting.");
	add_entry(pe, luw, LookedUpString); // Also nukes whitespace

	add_entry(pe, getwindowname(), FocusedWindowName);

	GPtrArray *dict = create_dictionary_from(luw);
	Stopif(dict->len == 0, free_pe(pe); g_ptr_array_free(dict, TRUE); return 1, "No dictionary entry found.");

	char   *def = NULL;
	size_t de_num = 0;
	int rv = popup(dict, &def, &de_num);
	add_entry(pe, def, DictionaryDefinition); // For freeing afterwards

	if (rv == 1)
	{
		add_entry(pe, luw, CopiedSentence);
		dictentry *chosen_dict = g_ptr_array_index(dict, de_num);
		add_entry(pe, chosen_dict->word, DictionaryKanji);

		ankicard ac;
		prepare_anki_card(&ac, pe);

		const char *err = addNote(ac);
		Stopif(err, , "Error creating note: %s", err);

		free(ac.fieldentries);
	}

	free_pe(pe);
	g_ptr_array_free(dict, TRUE);
}
