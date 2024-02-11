#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <pthread.h>

#include "xlib.h"
#include "popup.h"
#include "ankiconnectc.h"
#include "settings.h"
#include "deinflector.h"
#include "dbreader.h"

#ifndef UTIL_H
#include "util.h"
#endif

static int const number_of_possible_entries = 9;
typedef struct possible_entries_s {
	char* empty;
	char* lookup;
	char* copiedsentence;
	char* boldsentence;
	char* dictkanji;
	char* dictreading;
	char* dictdefinition;
	char* furigana;
	char* windowname;
	char* dictname;
} possible_entries_s;

static char*
map_entry(possible_entries_s pe, int i)
{
	// A safer way would be switching to strings, but I feel like that's
	// not very practical to configure
	return i == 0 ? NULL
	     : i == 1 ? pe.lookup
	     : i == 2 ? pe.copiedsentence
	     : i == 3 ? pe.boldsentence
	     : i == 4 ? pe.dictkanji
	     : i == 5 ? pe.dictreading
	     : i == 6 ? pe.dictdefinition
	     : i == 7 ? pe.furigana
	     : i == 8 ? pe.windowname
	     : i == 9 ? pe.dictname
	     : NULL;
}

static void
pe_free(possible_entries_s p)
{
	g_free(p.empty);
	g_free(p.lookup);
	g_free(p.copiedsentence);
	g_free(p.boldsentence);
	g_free(p.dictkanji);
	g_free(p.dictreading);
	g_free(p.dictdefinition);
	g_free(p.furigana);
	g_free(p.windowname);
}

char*
get_selection()
{
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	return gtk_clipboard_wait_for_text(clipboard);
}

static void
add_deinflections_to_dict(dictionary dict[static 1], s8 word)
{
	s8** deinflections = deinflect(word);

	assert(deinflections != NULL);
	for (s8** ptr = deinflections; *ptr; ptr++)
		add_word_to_dict(dict, **ptr);

	/* g_strfreev(deinflections); */
}

static void
fill_dictionary_with(dictionary dict[static 1], s8 word)
{
	add_word_to_dict(dict, word);
	add_deinflections_to_dict(dict, word);
}

static char*
boldWord(char sent[static 1], char* word)
{
	gchar   *bdword = g_strdup_printf("<b>%s</b>", word);
	GString *bdsent = g_string_new(sent);

	g_string_replace(bdsent, word, bdword, 0);

	g_free(bdword);
	return g_string_free_and_steal(bdsent);
}

static char*
create_furigana(char *kanji, char* reading)
{
	assert(kanji || reading);

	return !kanji ? g_strdup(reading)
	     : !reading ? NULL // Leave it to Anki
	     : !strcmp(kanji, reading) ? g_strdup(reading)
	     : g_strdup_printf("%s[%s]", kanji, reading); // TODO: Obviously not enough if kanji contains hiragana
}

static void
populate_entries(possible_entries_s pe[static 1], dictentry const de)
{
	if (cfg.copysentence)
	{
		notify(0, "Please select the context.");
		clipnotify();
		pe->copiedsentence = get_selection();
		if (cfg.nukewhitespace)
			nuke_whitespace(pe->copiedsentence);

		pe->boldsentence = boldWord(pe->copiedsentence, pe->lookup);
	}

	pe->dictdefinition = g_strdup(de.definition);
	pe->dictkanji = g_strdup(de.kanji ? de.kanji : de.reading);
	pe->dictreading = g_strdup(de.reading ? de.reading: de.kanji);
	pe->furigana = create_furigana(de.kanji, de.reading);
	pe->dictname = de.dictname;
}

static void
send_ankicard(possible_entries_s pe)
{
	ankicard ac = (ankicard) {
		.deck = cfg.deck,
		.notetype = cfg.notetype,
		.num_fields = (signed int)cfg.num_fields,
		.fieldnames = cfg.fieldnames,
		.fieldentries = g_new(char*, cfg.num_fields)
	};

	for (int i = 0; i < ac.num_fields; i++)
		ac.fieldentries[i] = map_entry(pe, cfg.fieldmapping[i]);

	check_ac_response(ac_addNote(ac));
	g_free(ac.fieldentries);
}

static void*
create_dictionary(void* voidin)
{
	char* lookupcstr = voidin;
	dictionary* dict = dictionary_new();

	s8 lookup = s8fromcstr(lookupcstr); // TODO: Convert somewhere earlier and switch to s8 below

	/* clock_t begin = clock(); */
	open_database();
	fill_dictionary_with(dict, lookup);
	if (dict->len == 0 && cfg.mecabconversion)
	{
		char *hira = kanji2hira(lookup);
		fill_dictionary_with(dict, s8fromcstr(hira));
		free(hira);
	}
	if (dict->len == 0 && cfg.substringsearch)
	{
		while (dict->len == 0 && lookup.len > 3) //FIXME: magic number. Change to > one UTF-8 character
		{
			lookup = s8striputf8chr(lookup);
			fill_dictionary_with(dict, lookup);
		}
	}
	close_database();
	/* clock_t end = clock(); */
	/* printf("lookup time: %f sec\n", (double)(end - begin) / CLOCKS_PER_SEC); */

	Stopif(dict->len == 0, dictionary_free(dict); exit(1), "No dictionary entry found.");

	dictionary_data_done(dict);
	return NULL;
}

int
main(int argc, char**argv)
{
	read_user_settings(number_of_possible_entries);

	gtk_init(&argc, &argv);

	possible_entries_s p = {
		.windowname = getwindowname(),
		.lookup = argc > 1 ? g_strdup(argv[1]) : get_selection()
	};

	Stopif(!p.lookup || !*p.lookup, exit(1), "No selection and no argument provided. Exiting.");

	if (cfg.nukewhitespace)
		nuke_whitespace(p.lookup);

	pthread_t thread;
	pthread_create(&thread, NULL, create_dictionary, p.lookup);

	dictentry* chosen_entry = popup();
	if (chosen_entry && cfg.ankisupport)
	{
		populate_entries(&p, *chosen_entry);
		dictentry_free(chosen_entry);
		send_ankicard(p);
	}

	pe_free(p);
	free_user_settings();
}
