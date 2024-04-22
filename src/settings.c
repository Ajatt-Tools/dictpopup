#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> // getopt

#include <glib.h>

#include "settings.h"
#include "util.h"
#include "messages.h"

static settings const default_cfg = {
    .general.sort = 0,
    .general.dictSortOrder = NULL,
    .general.dbpth = NULL, // Set by set_runtime_defaults()
    .general.nukeWhitespaceLookup = 1,
    .general.mecab = 0,
    .general.substringSearch = 1,
    //
    .anki.enabled = 0,
    .anki.deck = NULL, // Don't even try to guess
    .anki.notetype = "Japanese sentences",
    .anki.fieldnames = (char*[]){ "SentKanji", "VocabKanji", "VocabFurigana", "VocabDef", "Notes" },
    .anki.numFields = 0,
    .anki.copySentence = 1,
    .anki.nukeWhitespaceSentence = 1,
    .anki.fieldMapping = (u32[]){ 3, 4, 7, 6, 8 },
    .anki.checkExisting = 0,
    //
    .popup.width = 600,
    .popup.height = 400,
    .popup.margin = 5,
    //
    .pron.displayButton = 0,
    .pron.onStart = 0,
    .pron.dirPath = NULL, // Don't guess
};
settings cfg = default_cfg;
bool print_cfg = false;

static void
set_runtime_defaults(void)
{
    if (!cfg.general.dbpth)
    {
	cfg.general.dbpth = (char*)buildpath(fromcstr_((char*)g_get_user_data_dir()), S("dictpopup")).s;
	debug_msg("Using default database path: '%s'", cfg.general.dbpth);
    }
}

int
parse_cmd_line_opts(int argc, char** argv)
{
    int c;
    opterr = 0;

    while ((c = getopt(argc, argv, "dc")) != -1)
	switch (c)
	{
	case 'd':
	    cfg.args.debug = 1;
	    break;
	case 'c':
	    print_cfg = 1;
	    break;
	case '?':
	    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
	    exit(EXIT_FAILURE);
	default:
	    abort();
	}

    return optind;
}

#define printyn(boolean) \
	(boolean ? "yes" : "no")

// Expected to be null-terminated
static void
print_array(char** arr)
{
    putchar('[');
    if (arr)
    {
	for (char** p = arr; *p; p++)
	{
	    if (p != arr)
		putchar(',');
	    printf("\"%s\"", *p);
	}
    }
    putchar(']');
    putchar('\n');
}

void
print_settings(void)
{
    // TODO: Finish implementing
    puts("Settings:");
    puts("[General]");
    printf("Database path: '%s'\n", cfg.general.dbpth);
    printf("Sort: %s\n", printyn(cfg.general.sort));
    printf("Dictionary sort order: ");
    print_array(cfg.general.dictSortOrder);
    printf("Remove whitespace from lookup: %s\n", printyn(cfg.general.nukeWhitespaceLookup));
    printf("Fall back to hiragana conversion using MeCab: %s\n", printyn(cfg.general.mecab));
    printf("Decrese length until match: %s\n", printyn(cfg.general.substringSearch));
    putchar('\n');

    puts("[Anki]");
    printf("Anki support enabled: %s\n", printyn(cfg.anki.enabled));
    printf("Anki deck: \"%s\"\n", cfg.anki.deck);
    printf("Anki notetype: \"%s\"\n", cfg.anki.notetype);
    printf("Prompt to copy sentence: %s\n", printyn(cfg.anki.copySentence));
    printf("Remove whitespace from sentence: %s\n", printyn(cfg.anki.nukeWhitespaceSentence));
    printf("Fields: ");
    print_array(cfg.anki.fieldnames);
    printf("Mappings : [");
    for (size_t i = 0; i < cfg.anki.numFields; i++)
    {
	if (i)
	    putchar(',');
	printf("%i", cfg.anki.fieldMapping[i]);
    }
    printf("]\n");
    putchar('\n');

    puts("[Popup]");
    printf("Width: %i\n", cfg.popup.width);
    printf("Height: %i\n", cfg.popup.height);
    printf("Margin: %i\n", cfg.popup.margin);
    putchar('\n');

    puts("[Pronunciation]");
    printf("Display Button: %s\n", printyn(cfg.pron.displayButton));
    printf("Pronounce on start: %s\n", printyn(cfg.pron.onStart));
    printf("Audio file directory: %s\n", cfg.pron.dirPath ? cfg.pron.dirPath : "empty");
    putchar('\n');

    puts("Command line options:");
    printf("Debug output: %s\n", printyn(cfg.args.debug));

    putchar('\n');
}

/*
 * Read key @key from group @group into memory pointed to by @dest if exists and non-negative.
 * On error it will print a debug message and not write anything to @dest
 */
static void
read_uint(GKeyFile* kf, const char* group, const char* key, u32 dest[static 1])
{
    GError* error = NULL;
    int val = g_key_file_get_integer(kf, group, key, &error);
    if (error)
    {
	debug_msg("%s", error->message);
	g_error_free(error);
    }
    else if (val < 0)
	error_msg("Expected a positive value in key: '%s' and group '%s', but received negative.", key, group);
    else
	*dest = (u32)val;
}

/*
 * Read key @key from group @group and return the read value.
 * Returns @default_val on error
 */
static void
read_bool(GKeyFile* kf, const char* group, const char* key, bool dest[static 1])
{
    GError* error = NULL;
    gboolean value = g_key_file_get_boolean(kf, group, key, &error);
    if (error)
    {
	debug_msg("%s", error->message);
	g_error_free(error);
    }
    else
	*dest = value;
}

static void
read_string(GKeyFile* kf, const char* group, const char* key, char* dest[static 1])
{
    GError* error = NULL;
    char *value = g_key_file_get_string(kf, group, key, &error);
    if (error)
    {
	debug_msg("%s", error->message);
	g_error_free(error);
    }
    else
	*dest = value;
}

static void
read_string_list(GKeyFile* kf, const char* group, const char* key, char** dest[static 1], gsize* length)
{
    GError* error = NULL;
    char** value = g_key_file_get_string_list(kf, group, key, length ? length : NULL, &error);
    if (error)
    {
	debug_msg("%s", error->message);
	g_error_free(error);
    }
    else
	*dest = value;
}

static void
read_uint_list(GKeyFile* kf, const char* group, const char* key, u32* dest[static 1], gsize length[static 1])
{
    GError* error = NULL;
    int* val = g_key_file_get_integer_list(kf, group, key, length, &error);
    if (error)
    {
	debug_msg("%s", error->message);
	g_error_free(error);
	return;
    }

    for (gsize i = 0; i < *length; i++)
    {
	if (val[i] < 0)
	{
	    error_msg("Received a negative value at index '%li' in key: '%s' and group '%s' when expecting a positive.",
		      i, key, group);
	    return;
	}
    }

    *dest = (u32*)val;
}


static void
read_general(GKeyFile* kf)
{
    read_string(kf, "General", "DatabasePath", &cfg.general.dbpth);
    read_bool(kf, "General", "Sort", &cfg.general.sort);
    if (cfg.general.sort)
	read_string_list(kf, "General", "DictSortOrder", &cfg.general.dictSortOrder, NULL);
    read_bool(kf, "General", "NukeWhitespaceLookup", &cfg.general.nukeWhitespaceLookup);
    read_bool(kf, "General", "MecabConversion", &cfg.general.mecab);
    read_bool(kf, "General", "SubstringSearch", &cfg.general.substringSearch);
}

static void
read_anki(GKeyFile* kf)
{
    read_bool(kf, "Anki", "Enabled", &cfg.anki.enabled);
    if (cfg.anki.enabled)
    {
	read_string(kf, "Anki", "Deck", &cfg.anki.deck);
	read_string(kf, "Anki", "NoteType", &cfg.anki.notetype);

	size_t n_fields = 0;
	read_string_list(kf, "Anki", "FieldNames", &cfg.anki.fieldnames, &n_fields);

	read_bool(kf, "Anki", "CopySentence", &cfg.anki.copySentence);
	read_bool(kf, "Anki", "NukeWhitespaceSentence", &cfg.anki.nukeWhitespaceSentence);

	size_t n_fieldmap = 0;
	read_uint_list(kf, "Anki", "FieldMapping", &cfg.anki.fieldMapping, &n_fieldmap);

	if (n_fieldmap != n_fields)
	{
	    error_msg("Number of fieldnames does not match number of fieldmappings in config. Disabling Anki support..");
	    cfg.anki.enabled = 0;
	}
	else
	    cfg.anki.numFields = n_fields;

	read_bool(kf, "Anki", "CheckIfExists", &cfg.anki.checkExisting);
	read_string(kf, "Anki", "SearchField", &cfg.anki.searchField);
    }
}

static void
read_popup(GKeyFile* kf)
{
    read_uint(kf, "Popup", "Width", &cfg.popup.width);
    read_uint(kf, "Popup", "Height", &cfg.popup.height);
    read_uint(kf, "Popup", "Margin", &cfg.popup.margin);
}

static void
read_pronunciation(GKeyFile* kf)
{
    read_bool(kf, "Pronunciation", "DisplayButton", &cfg.pron.displayButton);
    read_bool(kf, "Pronunciation", "PronounceOnStart", &cfg.pron.onStart);
    read_string(kf, "Pronunciation", "FolderPath", &cfg.pron.dirPath);
}

void
read_user_settings(int fieldmapping_max)
{
    g_autoptr(GKeyFile) kf = g_key_file_new();
    GError *error = NULL;
    s8 cfgpth = buildpath(fromcstr_((char*)g_get_user_config_dir()), S("dictpopup"), S("config.ini"));
    if (!g_key_file_load_from_file(kf, (char*)cfgpth.s, G_KEY_FILE_NONE, &error))
    {
	if (g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
	    error_msg("Could not find a config file in: \"%s\". Falling back to default config.", cfgpth.s);
	else
	    error_msg("Error opening \"%s\": %s. Falling back to default config.", cfgpth.s, error->message);

	g_error_free(error);
	error = NULL;
	return;
    }
    else
    {
	read_general(kf);
	read_anki(kf);
	read_popup(kf);
	read_pronunciation(kf);
    }
    frees8(&cfgpth);

    set_runtime_defaults();

    if (print_cfg)
      print_settings();
}
