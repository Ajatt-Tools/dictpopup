#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <ankiconnectc.h>
#include <pthread.h>

#include "xlib.h"
#include "util.h"
#include "deinflector.h"
#include "popup.h"
#include "readsettings.h"
#include "dictionary.h"
#include "dictionarylookup.h"
#include "kanaconv.h"

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

//Might be better to switch to a struct:
/* typedef struct possible_entries_s { */
/* 	char *empty; */
/* 	char *lookup; */
/* 	char *deinflected_lookup; */
/* 	char *copied_sentence; */
/* 	char *bold_sentence; */
/* 	char *dictionary_kanji; */
/* 	char *dictionary_reading; */
/* 	char *dictionary_definition; */
/* 	char *deinflected_furigana; */
/* 	char *windowname; */
/* } possible_entries_s; */

typedef struct {
	char *p[NUMBER_POSS_ENTRIES];
	dictionary* dict;
} SharedData;

void
add_deinflections_to_dict(dictionary *dict, char *word)
{
	char **deinflections = deinflect(word);

	for (char **ptr = deinflections; *ptr; ptr++)
		add_word_to_dict(dict, *ptr);

	g_strfreev(deinflections);
}

void
fill_dictionary_with(dictionary* dict, char *word)
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
	SharedData *in = voidin;
	char **p = in->p;
	dictionary* dict = in->dict;

	p[FocusedWindowName] = getwindowname();

	fill_dictionary_with(dict, p[LookedUpString]);

	if (dict->len == 0)
	{   // Try hiragana conversion as a last resort. Meant for lookups like 思いつく
	    char *hira = kanji2hira(p[LookedUpString]);
	    fill_dictionary_with(dict, hira);
	    free(hira);
	}

	Stopif(dict->len == 0, p_free(p); dictionary_free(dict); exit(1), "No dictionary entry found.");

	dictionary_data_done();
	return NULL;
}

void*
display_popup(void *voidin)
{
	SharedData *in = voidin;
	char **p = in->p;
	dictionary *dict = in->dict;

	size_t de_num;
	if (popup(dict, &p[DictionaryDefinition], &de_num) && cfg.ankisupport)
	{
		dictentry *chosen_dict = dictentry_at_index(dict, de_num);
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

	SharedData data = (SharedData) { .dict = dictionary_new(), .p = { 0 } };

	data.p[LookedUpString] = argc > 1 ? g_strdup(argv[1]) : sselp(); //strdup, so can be freed afterwards
	Stopif(!data.p[LookedUpString] || !*data.p[LookedUpString], exit(1), "No selection and no argument provided. Exiting.");
	if (cfg.nukewhitespace)
		nuke_whitespace(data.p[LookedUpString]);

	pthread_t threads[2];
	pthread_create(&threads[0], NULL, lookup_and_create_dictionary, &data);
	pthread_create(&threads[1], NULL, display_popup, &data);
	for (int i = 0; i < 2; i++)
		pthread_join(threads[i], NULL);

	p_free(data.p);
	dictionary_free(data.dict);
	free_user_settings();
}
