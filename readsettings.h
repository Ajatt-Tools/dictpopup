#ifndef SETTINGS_H
#define SETTINGS_H

#define NUMBER_POSS_ENTRIES 10

typedef struct {
	// Anki
	char *deck;
	char *notetype;
	char **fieldnames;
	size_t num_fields;
	int *fieldmapping;
	char *searchfield;
	// Popup
	int win_width;
	int win_height;
	int win_margin;
	// Database
	char* db_path;
	// Behaviour
	gboolean ankisupport;
	gboolean checkexisting;
	gboolean copysentence;
	gboolean nukewhitespace;
	gboolean pronunciationbutton;
	gboolean mecabconversion;
	gboolean substringsearch;
	gboolean pronounceonstart;
} settings;

extern settings cfg;

void print_settings();
void read_user_settings();
void free_user_settings();

#endif /* SETTINGS_H */
