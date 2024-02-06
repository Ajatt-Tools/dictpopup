#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "util.h"
#include "readsettings.h"

#define printbool(boolean) \
	(boolean ? "true" : "false")

settings cfg = { 0 };


void
free_user_settings()
{
	// TODO: Implement
	g_free(cfg.db_path);
}

void
print_settings()
{
	printf("[Anki]\n");
	printf("Anki deck: \"%s\"\n", cfg.deck);
	printf("Anki Notetype: \"%s\"\n", cfg.notetype);
	printf("Fields: [");
	for (size_t i = 0; i < cfg.num_fields; i++)
	{
		printf("\"%s\"", cfg.fieldnames[i]);
		if (i != cfg.num_fields - 1)
			printf(",");
		else
			printf("]\n");
	}
	printf("Mappings : [");
	for (size_t i = 0; i < cfg.num_fields; i++)
	{
		printf("%i", cfg.fieldmapping[i]);
		if (i != cfg.num_fields - 1)
			printf(",");
		else
			printf("]\n");
	}
	printf("\n");
	printf("[Behaviour]\n");
	printf("Anki support: %s\n", printbool(cfg.ankisupport));
	printf("Check if existing: %s\n", printbool(cfg.checkexisting));
	printf("Copy sentence: %s\n", printbool(cfg.copysentence));
	printf("Nuke whitespace: %s\n", printbool(cfg.nukewhitespace));
}

// TODO: Only notification on critical warning. Not missing window size
static void
error_msg(char const *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	g_autofree char* msg = g_strdup_vprintf(fmt, argp);
	va_end(argp);

	notify(1, "WARNING: %s", msg);
	g_warning(msg);
}

void
check_fieldmapping(size_t num, unsigned int fieldmapping[num], unsigned int max_value)
{
	for (size_t i = 0; i < num; i++)
	{
		if (fieldmapping[i] >= max_value)
		{
			error_msg("Found a field mapping outside of the defined range in the config file. Setting to 0");
			fieldmapping[i] = 0;
		}
	}
}

static void
fill_anki_string(GKeyFile* kf, char **cfg_option, const char* entry)
{
	if (cfg.ankisupport)
	{
		g_autoptr(GError) error = NULL;
		char *value = g_key_file_get_string(kf, "Anki", entry, &error);
		if (value == NULL || !*value)
		{
			if (error)
				error_msg("%s. Disabling Anki support.", error->message);
			else
				error_msg("Missing entry \"%s\" in settings file. Disabling Anki support", entry);

			cfg.ankisupport = 0;
		}
		else
			*cfg_option = value;
	}
}


static gboolean
return_behaviour(GKeyFile* kf, const char* entry, gboolean defaultv)
{
	g_autoptr(GError) error = NULL;
	gboolean value = g_key_file_get_boolean(kf, "Behaviour", entry, &error);
	if (error)
	{
		error_msg("%s. Setting \"%s\" to \"%s\".", error->message, entry, printbool(defaultv));
		return defaultv;
	}

	return value;
}

static unsigned int
return_popup(GKeyFile* kf, const char* entry, int defaultv)
{
	g_autoptr(GError) error = NULL;
	int value = g_key_file_get_integer(kf, "Popup", entry, &error);

	return error? defaultv : value;
}

void
read_user_settings(unsigned int fieldmapping_max)
{
	const char* config_dir = g_get_user_config_dir();
	g_autofree gchar *config_fn = g_build_filename(config_dir, "dictpopup", "config.ini", NULL);

	GError *error = NULL;
	g_autoptr(GKeyFile) kf = g_key_file_new();
	if (!g_key_file_load_from_file(kf, config_fn, G_KEY_FILE_NONE, &error))
	{
		if (g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
			error_msg("Could not find config file: \"%s\"", config_fn);
		else
			error_msg("Error opening \"%s\": %s.", config_fn, error->message);

		g_error_free(error);
		exit(1); // TODO: default config
	}

	cfg.ankisupport = return_behaviour(kf, "AnkiSupport", false);
	cfg.checkexisting = return_behaviour(kf, "CheckIfExists", false);
	cfg.copysentence = return_behaviour(kf, "CopySentence", false);
	cfg.nukewhitespace = return_behaviour(kf, "NukeWhitespace", true);
	cfg.pronunciationbutton = return_behaviour(kf, "PronunciationButton", false);
	cfg.pronounceonstart = return_behaviour(kf, "PronounceOnStart", false);
	cfg.mecabconversion = return_behaviour(kf, "MecabConversion", false);
	cfg.substringsearch = return_behaviour(kf, "SubstringSearch", true);

	fill_anki_string(kf, &(cfg.deck), "Deck");
	fill_anki_string(kf, &(cfg.notetype), "NoteType");
	if (cfg.ankisupport && cfg.checkexisting)
	{
		char *value = g_key_file_get_string(kf, "Anki", "SearchField", &error);
		if (value == NULL || !*value)
		{
			if (error)
			{
				error_msg("%s. Disabling existence searching.", error->message);
				g_error_free(error);
				error = NULL;
			}
			else
				error_msg("Missing entry \"SearchField\" in settings file. Disabling existence searching.");

			cfg.checkexisting = 0;
		}
		else
			cfg.searchfield = value;
	}

	// Fieldnames
	size_t num_fieldnames = 0;
	if (cfg.ankisupport)
	{
		cfg.fieldnames = g_key_file_get_string_list(kf, "Anki", "FieldNames", &num_fieldnames, &error);
		cfg.num_fields = num_fieldnames;
		if (error)
		{
			error_msg("%s. Disabling Anki support.", error->message);
			cfg.ankisupport = FALSE;
			g_error_free(error);
			error = NULL;
		}
	}

	// Fieldmappings
	size_t num_fieldmappings = 0;
	if (cfg.ankisupport)
	{
		// Negative values are changed to positive ones, but then set to 0, as they will be out of range
		cfg.fieldmapping = (unsigned int*)g_key_file_get_integer_list(kf, "Anki", "FieldMapping", &num_fieldmappings, &error);
		if (error)
		{
			error_msg("%s. Disabling Anki support.", error->message);
			cfg.ankisupport = FALSE;
			g_error_free(error);
			error = NULL;
		}
		check_fieldmapping(num_fieldmappings, cfg.fieldmapping, fieldmapping_max);

		Stopif(num_fieldnames != num_fieldmappings, exit(1), "Error: Number of fieldnames does not match number of fieldmappings in config.");
	}

	// Database
	cfg.db_path = g_key_file_get_string(kf, "Database", "Path", &error);
	if (!cfg.db_path || !*cfg.db_path)
	{
		// silently use the default path
		const char* data_dir = g_get_user_data_dir();
		cfg.db_path = g_build_filename(data_dir, "dictpopup", NULL);
	}

	// Popup
	cfg.win_height = return_popup(kf, "Height", 350);
	cfg.win_width = return_popup(kf, "Width", 500);
	cfg.win_margin = return_popup(kf, "Margin", 5);
}
