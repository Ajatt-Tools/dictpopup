#include <pthread.h>
#include "settings.h"
#include "util.h"
#include "ankiconnectc.h"
#include "messages.h"
#include "dictlookup.h"
#include "popup.h"

#define POSSIBLE_ENTRIES_S_NMEMB 9
typedef struct possible_entries_s {
    s8 lookup;
    s8 copiedsent;
    s8 boldsent;
    s8 dictkanji;
    s8 dictreading;
    s8 dictdefinition;
    s8 furigana;
    s8 windowname;
    s8 dictname;
} possible_entries_s;

static char*
map_entry(possible_entries_s pe, int i)
{
    // A safer way would be switching to strings, but I feel like that's
    // not very practical to configure
    // TODO: s8 implemenation?
    // TODO: Warning if unknown number encountered
    s8 ret = i == 0 ? (s8){ 0 }
	     : i == 1 ? pe.lookup
	     : i == 2 ? pe.copiedsent
	     : i == 3 ? pe.boldsent
	     : i == 4 ? pe.dictkanji
	     : i == 5 ? pe.dictreading
	     : i == 6 ? pe.dictdefinition
	     : i == 7 ? pe.furigana
	     : i == 8 ? pe.windowname
	     : i == 9 ? pe.dictname
	     : (s8){ 0 };

    assert(ret.s == NULL || ret.s[ret.len] == '\0');
    return (char*)ret.s;
}

#include <glib.h>
static s8
add_bold_tags(s8 sent, s8 word)
{
    s8 bdword = concat(S("<b>"), word, S("</b>"));

    GString* bdsent = g_string_new_len((gchar*)sent.s, (gssize)sent.len);
    assert(word.s[word.len] == '\0');
    assert(bdword.s[bdword.len] == '\0');
    g_string_replace(bdsent, (char*)word.s, (char*)bdword.s, 0);

    frees8(&bdword);

    s8 ret = { 0 };
    ret.len = bdsent->len;
    ret.s = (u8*)g_string_free_and_steal(bdsent);
    return ret;
}

static s8
create_furigana(s8 kanji, s8 reading)
{
    return !kanji.len && !reading.len ? (s8){ 0 }
	     : !reading.len ? (s8){ 0 } // Leave it to AJT Japanese
	     : !kanji.len ? s8dup(reading)
	     : s8equals(kanji, reading) ? s8dup(reading)
	     : concat(kanji, S("["), reading, S("]")); // TODO: Obviously not enough if kanji contains hiragana
}


static void
send_ankicard(possible_entries_s p)
{
    ankicard ac = (ankicard) {
	.deck = cfg.anki.deck,
	.notetype = cfg.anki.notetype,
	.num_fields = cfg.anki.numFields,
	.fieldnames = cfg.anki.fieldnames,
	.fieldentries = alloca(cfg.anki.numFields * sizeof(char*))
    };

    for (size_t i = 0; i < ac.num_fields; i++)
	ac.fieldentries[i] = map_entry(p, cfg.anki.fieldMapping[i]);

    retval_s ac_resp = ac_addNote(ac);
    if (ac_resp.ok)
	msg("Successfully added card.");
    else
	error_msg("Error adding card: %s", ac_resp.data.string);
}

static void
fill_entries(possible_entries_s pe[static 1], dictentry const de)
{
    if (cfg.anki.copySentence)
    {
	msg("Please select the context.");
	pe->copiedsent = get_sentence();
	if (cfg.anki.nukeWhitespaceSentence)
	    pe->copiedsent = nuke_whitespace(pe->copiedsent);

	pe->boldsent = add_bold_tags(pe->copiedsent, pe->lookup);
    }

    pe->dictdefinition = s8dup(de.definition);
    pe->dictkanji = s8dup(de.kanji.len > 0 ? de.kanji : de.reading);
    pe->dictreading = s8dup(de.reading.len > 0 ? de.reading : de.kanji);
    pe->furigana = create_furigana(de.kanji, de.reading);
    pe->dictname = de.dictname;
}

void*
create_dictionary_caller(void* voidin)
{
   s8* lookup = (s8*)voidin;
   create_dictionary(lookup);
   return NULL;
}

int
main(int argc, char** argv)
{
    int nextarg = parse_cmd_line_opts(argc, argv);
    read_user_settings(POSSIBLE_ENTRIES_S_NMEMB);
    if (cfg.args.debug)
	print_settings();

    possible_entries_s p = { 0 };
    p.windowname = get_windowname();
    p.lookup = argc - nextarg > 0 ? fromcstr_(argv[nextarg]) : get_selection();
    if (p.lookup.len == 0)
	fatal("No selection and no argument provided. Exiting.");
    if (!g_utf8_validate((char*)p.lookup.s, p.lookup.len, NULL))
	fatal("Lookup is not a valid UTF-8 string");
    if (cfg.general.nukeWhitespaceLookup)
	nuke_whitespace(p.lookup);

    pthread_t thread;
    pthread_create(&thread, NULL, create_dictionary_caller, &p.lookup);
    pthread_detach(thread);

    dictentry chosen_entry = popup();
    if (chosen_entry.definition.len && cfg.anki.enabled)
    {
	fill_entries(&p, chosen_entry);
	send_ankicard(p);
    }
}
