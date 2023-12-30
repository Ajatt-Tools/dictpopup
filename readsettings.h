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
	// Behaviour
	gboolean ankisupport;
	gboolean checkexisting;
	gboolean copysentence;
	gboolean nukewhitespace;
	gboolean pronunciationbutton;
} settings;

extern settings cfg;

void print_settings();
void read_user_settings();
void settings_free();

#endif /* SETTINGS_H */
