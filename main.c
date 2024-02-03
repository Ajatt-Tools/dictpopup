#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <pthread.h>

#include "xlib.h"
#include "util.h"
#include "deinflector.h"
#include "popup.h"
#include "readsettings.h"
#include "dictionary.h"
#include "dictionarylookup.h"
#include "kanaconv.h"
#include "ankiconnectc.h"

enum PossibleEntries {
	Empty,
	LookedUpString,
	CopiedSentence,
	BoldSentence,
	DictionaryKanji,
	DictionaryReading,
	DictionaryDefinition,
	DeinflectedFurigana,
	FocusedWindowName
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
	if (!kanji)
		return g_strdup(reading);
	else if (!reading)
		return g_strdup(kanji);

	// TODO: The following is obviously not enough if kanji contains hiragana
	return strcmp(kanji, reading) == 0 ? g_strdup(kanji) : g_strdup_printf("%s[%s]", kanji, reading);
}

void
populate_entries(char *pe[], dictentry* de)
{
	if (cfg.copysentence)
	{
		notify(0, "Please select the context.");
		clipnotify();
		pe[CopiedSentence] = sselp();
		if (cfg.nukewhitespace)
			nuke_whitespace(pe[CopiedSentence]);
	}

	pe[BoldSentence] = boldWord(pe[CopiedSentence], pe[LookedUpString]);

	pe[DictionaryReading] = g_strdup(de->reading ? de->reading: de->kanji);

	pe[DictionaryKanji] = g_strdup(de->kanji ? de->kanji : de->reading);

	pe[DeinflectedFurigana] = create_furigana(pe[DictionaryKanji], pe[DictionaryReading]);
}

void
check_addNote_response(const char *err)
{
	if (err)
		notify(1, "Error creating card: %s", err);
	else
		notify(0, "Sucessfully added card.");
}

void
send_ankicard(char **pe)
{
	ankicard ac = (ankicard) {
		.deck = cfg.deck,
		.notetype = cfg.notetype,
		.num_fields = cfg.num_fields,
		.fieldnames = cfg.fieldnames,
		.fieldentries = g_new(char*, cfg.num_fields)
	};

	for (int i = 0; i < ac.num_fields; i++)
		ac.fieldentries[i] = pe[cfg.fieldmapping[i]];

	const char *err = ac_addNote(&ac, check_addNote_response);
	if (err)
		notify(1, "%s", err);

	g_free(ac.fieldentries);
}

void
p_free(char **p)
{
	for (int i = 0; i < NUMBER_POSS_ENTRIES; i++)
		g_free(p[i]);
}

void*
create_dictionary(void *voidin)
{
	SharedData *in = voidin;
	char **p = in->p;
	dictionary* dict = in->dict;

	p[FocusedWindowName] = getwindowname();

	clock_t begin = clock();
	open_database();

	fill_dictionary_with(dict, p[LookedUpString]);
	if (dict->len == 0 && cfg.mecabconversion)
	{
		char *hira = kanji2hira(p[LookedUpString]);
		fill_dictionary_with(dict, hira);
		free(hira);
	}
	if (dict->len == 0 && cfg.substringsearch)
	{
		/* size_t longest_entry_len = get_longest_entry_len(); */
		int lookup_strlen = strlen(p[LookedUpString]);
		/* if (lookup_strlen > longest_entry_len) */
		/*   remove_last_unichar(p[LookedUpString], longest_entry_len + 1); */
		while (dict->len == 0 && lookup_strlen > 3) //FIXME: magic number
		{
			remove_last_unichar(p[LookedUpString], &lookup_strlen);
			fill_dictionary_with(dict, p[LookedUpString]);
		}
	}

	close_database();
	clock_t end = clock();
	printf("Time taken for lookup: %f sec\n", (double)(end - begin) / CLOCKS_PER_SEC);

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

		send_ankicard(p);
	}

	return NULL;
}

int
main(int argc, char**argv)
{
	gtk_init(&argc,&argv);

	clock_t begin = clock();
	read_user_settings();
	clock_t end = clock();
	printf("Time taken to read user settings: %f sec\n", (double)(end - begin) / CLOCKS_PER_SEC);

	SharedData data = (SharedData) { .dict = dictionary_new(), .p = { 0 } };
	data.p[LookedUpString] = argc > 1 ? g_strdup(argv[1]) : sselp();

	Stopif(!data.p[LookedUpString] || !*data.p[LookedUpString], exit(1), "No selection and no argument provided. Exiting.");
	if (cfg.nukewhitespace)
		nuke_whitespace(data.p[LookedUpString]);

	pthread_t threads[2];
	pthread_create(&threads[0], NULL, create_dictionary, &data);
	pthread_create(&threads[1], NULL, display_popup, &data);
	for (int i = 0; i < 2; i++)
		pthread_join(threads[i], NULL);

	p_free(data.p);
	dictionary_free(data.dict);
	free_user_settings();
}
