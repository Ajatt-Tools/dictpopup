#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "util.h"
#include "readsettings.h"

#define printbool(boolean) \
	boolean ? "TRUE" : "FALSE"

settings cfg = { 0 };

void
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
free_user_settings()
{
	// TODO: Implement
}

void
print_settings()
{
	printf("[Anki]\n");
	printf("Anki deck: \"%s\"\n", cfg.deck);
	printf("Anki Notetype: \"%s\"\n", cfg.notetype);
	printf("Fields: [");
	for (int i = 0; i < cfg.num_fields; i++)
	{
		printf("\"%s\"", cfg.fieldnames[i]);
		if (i != cfg.num_fields - 1)
			printf(",");
		else
			printf("]\n");
	}
	printf("Mappings : [");
	for (int i = 0; i < cfg.num_fields; i++)
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

void
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


void
fill_behaviour(GKeyFile* kf, gboolean *cfg_option, const char* entry, gboolean defaultv)
{
	g_autoptr(GError) error = NULL;
	gboolean value = g_key_file_get_boolean(kf, "Behaviour", entry, &error);
	if (error)
	{
		error_msg("%s. Setting \"%s\" to %s.", error->message, entry, defaultv ? "true" : "false");
		*cfg_option = defaultv;
	}
	else
		*cfg_option = value;
}

void
fill_popup(GKeyFile* kf, int *cfg_option, const char* entry, int default_value)
{
	g_autoptr(GError) error = NULL;
	int value = g_key_file_get_integer(kf, "Popup", entry, &error);
	if (error)
	{
		error_msg("%s. Defaulting to \"%i\".", error->message, default_value);
		*cfg_option = default_value;
	}
	else
		*cfg_option = value;
}

void
read_user_settings()
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

	fill_behaviour(kf, &(cfg.ankisupport), "AnkiSupport", FALSE);
	fill_behaviour(kf, &(cfg.checkexisting), "CheckIfExists", FALSE);
	fill_behaviour(kf, &(cfg.copysentence), "CopySentence", FALSE);
	fill_behaviour(kf, &(cfg.nukewhitespace), "NukeWhitespace", TRUE);
	fill_behaviour(kf, &(cfg.pronunciationbutton), "PronunciationButton", FALSE);
	fill_behaviour(kf, &(cfg.pronounceonstart), "PronounceOnStart", FALSE);
	fill_behaviour(kf, &(cfg.mecabconversion), "MecabConversion", FALSE);
	fill_behaviour(kf, &(cfg.substringsearch), "SubstringSearch", TRUE);

	fill_anki_string(kf, &(cfg.deck), "Deck");
	fill_anki_string(kf, &(cfg.notetype), "NoteType");
	if (cfg.checkexisting && cfg.ankisupport)
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
		cfg.fieldmapping = g_key_file_get_integer_list(kf, "Anki", "FieldMapping", &num_fieldmappings, &error);
		if (error)
		{
			error_msg("%s. Disabling Anki support.", error->message);
			cfg.ankisupport = FALSE;
			g_error_free(error);
			error = NULL;
		}

		Stopif(num_fieldnames != num_fieldmappings, exit(1), "Error: Number of fieldnames does not match number of fieldmappings.");
	}

	// Database
	cfg.db_path = g_key_file_get_string(kf, "Database", "Path", &error);
	if (error)
	{
	  //TODO: Implement
	}

	// Popup
	fill_popup(kf, &(cfg.win_height), "Height", 350);
	fill_popup(kf, &(cfg.win_width), "Width", 500);
	fill_popup(kf, &(cfg.win_margin), "Margin", 5);
}
