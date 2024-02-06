#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct {
	// Anki
	char* deck;
	char* notetype;
	char** fieldnames;
	size_t num_fields;
	unsigned int* fieldmapping;
	char* searchfield;
	// Popup
	unsigned int win_width;
	unsigned int win_height;
	unsigned int win_margin;
	// Database
	char* db_path;
	// Behaviour
	unsigned int ankisupport : 1;
	unsigned int checkexisting : 1;
	unsigned int copysentence : 1;
	unsigned int nukewhitespace : 1;
	unsigned int pronunciationbutton : 1;
	unsigned int mecabconversion : 1;
	unsigned int substringsearch : 1;
	unsigned int pronounceonstart : 1;
} settings;

extern settings cfg;

void print_settings();
void read_user_settings();
void free_user_settings();

#endif /* SETTINGS_H */
