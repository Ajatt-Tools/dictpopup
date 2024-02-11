#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct {
	// Anki
	char* deck;
	char* notetype;
	char** fieldnames;
	size_t num_fields;
	int* fieldmapping;
	char* searchfield;
	// Popup
	int win_width;
	int win_height;
	int win_margin;
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
void read_user_settings(int fieldmapping_max);
void free_user_settings();

#endif /* SETTINGS_H */
