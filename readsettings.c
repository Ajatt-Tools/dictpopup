#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "util.h"
#include "readsettings.h"

#define printbool(boolean) \
	boolean ? "TRUE" : "FALSE"

settings cfg;

/*
 * Checks fieldmapping for validity
 * Returns: TRUE for valid and FALSE for invalid.
 */
gboolean
validate_fieldmapping(int fieldmapping[], size_t len)
{
	for (int i = 0; i < len; i++)
		if (fieldmapping[i] < 0 || fieldmapping[i] >= NUMBER_POSS_ENTRIES)
			return FALSE;
	return TRUE;
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
				notify("WARNING: %s. Disabling Anki support.", error->message);
			else
				notify("WARNING: Missing entry \"%s\" in settings file. Disabling Anki support", entry);

			cfg.ankisupport = 0;
		}
		else
			*cfg_option = value;
	}
}

void
fill_behaviour(GKeyFile* kf, gboolean *cfg_option, const char* entry)
{
	g_autoptr(GError) error = NULL;
	gboolean value = g_key_file_get_boolean(kf, "Behaviour", entry, &error);
	if (error)
	{
		notify("Error: %s. Setting \"%s\" to True.", error->message, entry);
		g_warning("Error: %s. Setting \"%s\" to True.", error->message, entry);
		*cfg_option = TRUE;
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
		notify("Error: %s. Defaulting to \"%i\".", error->message, default_value);
		g_warning("Error: %s. Defaulting to \"%i\".", error->message, default_value);
		*cfg_option = default_value;
	}
	else
		*cfg_option = value;
}

void
read_user_settings()
{
	const char* config_dir = g_get_user_config_dir();
	const gchar *config_fn = g_build_filename(config_dir, "dictpopup", "config.ini", NULL);

	GError *error = NULL;
	g_autoptr(GKeyFile) kf = g_key_file_new();
	if (!g_key_file_load_from_file(kf, config_fn, G_KEY_FILE_NONE, &error))
	{
		if (g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
		{
			notify("Could not find config file: \"%s\"", config_fn);
			g_warning("Could not find config file: \"%s\"", config_fn);
		}
		else
		{
			notify("Error opening \"%s\": %s.", config_fn, error->message); g_warning("Error opening \"%s\": %s.", config_fn, error->message);
		}

		g_error_free(error);
		exit(1);
	}
	fill_behaviour(kf, &(cfg.ankisupport), "AnkiSupport");
	fill_behaviour(kf, &(cfg.checkexisting), "CheckIfExists");
	fill_behaviour(kf, &(cfg.copysentence), "CopySentence");
	fill_behaviour(kf, &(cfg.nukewhitespace), "NukeWhitespace");
	fill_behaviour(kf, &(cfg.pronunciationbutton), "PronunciationButton");

	fill_anki_string(kf, &(cfg.deck), "Deck");
	fill_anki_string(kf, &(cfg.notetype), "NoteType");
	if (cfg.checkexisting && cfg.ankisupport)
	{
		char *value = g_key_file_get_string(kf, "Anki", "SearchField", &error);
		if (value == NULL || !*value)
		{
			if (error)
			{
				notify("WARNING: %s. Disabling existence searching.", error->message);
				g_warning("WARNING: %s. Disabling existence searching.", error->message);
				g_error_free(error);
				error = NULL;
			}
			else
			{
				notify("WARNING: Missing entry \"SearchField\" in settings file. Disabling existence searching.");
				g_warning("WARNING: Missing entry \"SearchField\" in settings file. Disabling existence searching.");
			}
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
			notify("WARNING: %s. Disabling Anki support.", error->message);
			g_warning("%s. Disabling Anki support.", error->message);
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
			notify("WARNING: %s. Disabling Anki support.", error->message);
			g_warning("%s. Disabling Anki support.", error->message);
			cfg.ankisupport = FALSE;
			g_error_free(error);
			error = NULL;
		}
		if (!validate_fieldmapping(cfg.fieldmapping, num_fieldmappings))
		{
			notify("WARNING: Encountered an invalid value in FieldMapping setting. Disabling Anki support.");
			g_warning("Encountered an invalid value in FieldMapping setting. Disabling Anki support.");
			cfg.ankisupport = FALSE;
		}

		Stopif(num_fieldnames != num_fieldmappings, exit(1), "Error: Number of fieldnames does not match number of fieldmappings.");
	}

	// Popup
	fill_popup(kf, &(cfg.win_height), "Height", 350);
	fill_popup(kf, &(cfg.win_width), "Width", 500);
	fill_popup(kf, &(cfg.win_margin), "Margin", 5);
}
