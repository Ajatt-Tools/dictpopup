#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "util.h"
#include "readsettings.h"

#define printbool(boolean) \
	boolean ? "TRUE" : "FALSE"

void
print_settings(settings *cfg)
{
	printf("[Anki]\n");
	printf("Anki deck: \"%s\"\n", cfg->deck);
	printf("Anki Notetype: \"%s\"\n", cfg->notetype);
	printf("Fields: [");
	for (int i = 0; i < cfg->num_fields; i++)
	{
		printf("\"%s\"", cfg->fieldnames[i]);
		if (i != cfg->num_fields - 1)
			printf(",");
		else
			printf("]\n");
	}
	printf("Mappings : [");
	for (int i = 0; i < cfg->num_fields; i++)
	{
	  printf("%i", cfg->fieldmapping[i]);
	  if (i != cfg->num_fields-1) printf(",");
	  else printf("]\n");
	}
	printf("\n");
	printf("[Behaviour]\n");
	printf("Anki support: %s\n", printbool(cfg->ankisupport));
	printf("Check if existing: %s\n", printbool(cfg->checkexisting));
	printf("Copy sentence: %s\n", printbool(cfg->copysentence));
	printf("Nuke whitespace: %s\n", printbool(cfg->nukewhitespace));
}

void
fill_anki_string(GKeyFile* kf, char **cfg_option, gboolean* ankisupport, const char* entry)
{
	if (*ankisupport)
	{
		g_autoptr(GError) error = NULL;
		char *value = g_key_file_get_string(kf, "Anki", entry, &error);
		if (value == NULL)
		{
			if (g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND))
				g_warning("Error finding Anki group in settings file: %s. \
			      You need to define your Anki settings under [Anki]. Disabling Anki support.", error->message);
			if (g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND))
				g_warning("Error finding Anki %s in settings file: %s under [Anki]. Disabling Anki support.", entry, error->message);
			else
				g_warning("No Anki %s provided in settings file: %s. Disabling Anki support", entry, error->message);

			*ankisupport = 0;
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
	if (error != NULL)
	{
		if (g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND))
			g_warning("Error finding Behaviour group in settings file: %s. \
			      You need to define behaviour settings under [Behaviour]. Setting %s to True.", error->message, entry);
		else if (g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND))
			g_warning("Error finding %s in settings file: %s under [Behaviour]. Disabling Anki support.", entry, error->message);
		else if (g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE))
			g_warning("Received an invalid value in setting %s. Only true or false allowed. Setting to true.", entry);
		*cfg_option = TRUE;
	}
	else
	{
		*cfg_option = value;
	}
}

settings*
read_user_settings()
{
	settings *cfg = malloc(sizeof(settings));

	const char* config_dir = g_get_user_config_dir();
	const gchar *config_fn = g_build_filename(config_dir, "dictpopup", "config.ini", NULL);

	g_autoptr(GError) error = NULL;
	g_autoptr(GKeyFile) kf = g_key_file_new();
	if (!g_key_file_load_from_file(kf, config_fn, G_KEY_FILE_NONE, &error))
	{
		if (g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
			g_warning("Error loading key file: %s", error->message);
		exit(1);
	}


	fill_behaviour(kf, &(cfg->ankisupport), "EnableAnkiSupport");
	fill_behaviour(kf, &(cfg->checkexisting), "CheckIfExists");
	fill_behaviour(kf, &(cfg->copysentence), "CopySentence");
	fill_behaviour(kf, &(cfg->nukewhitespace), "NukeWhitespace");

	fill_anki_string(kf, &(cfg->deck), &(cfg->ankisupport), "Deck");
	fill_anki_string(kf, &(cfg->notetype), &(cfg->ankisupport), "NoteType");
	if (cfg->checkexisting)
		fill_anki_string(kf, &(cfg->searchfield), &(cfg->ankisupport), "SearchField");

	size_t num_fieldnames;
	cfg->fieldnames = g_key_file_get_string_list(kf, "Anki", "FieldNames", &num_fieldnames, &error);
	cfg->num_fields = num_fieldnames;

	size_t num_fieldmappings;
	cfg->fieldmapping = g_key_file_get_integer_list(kf, "Anki", "FieldMapping", &num_fieldmappings, &error);
	if (error != NULL)
	{
		if (g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND))
			g_warning("Error finding \"FieldMapping\" in settings file: %s under [Anki]. Disabling Anki support.", error->message);
		else if (g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE))
			g_warning("Received an invalid value in setting \"FieldMapping\". Only numbers allowed. Disabling Anki support.");
		else
			g_warning("Error in reading the setting \"FieldMapping\" occured: %s. Disabling Anki support.", error->message);
		cfg->ankisupport = FALSE;
	}

	Stopif(num_fieldnames != num_fieldmappings, exit(1), "ERROR: Number of fieldnames does not match number of fieldmappings.");

	return cfg;
}
