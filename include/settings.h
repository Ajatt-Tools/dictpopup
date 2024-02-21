#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct {
	// General
	char* db_path;
	char** sort_order;
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
	// Behaviour
	unsigned int ankisupport : 1;
	unsigned int checkexisting : 1;
	unsigned int copysentence : 1;
	unsigned int nukewhitespace : 1;
	unsigned int pronunciationbutton : 1;
	unsigned int mecabconversion : 1;
	unsigned int substringsearch : 1;
	unsigned int pronounceonstart : 1;
	unsigned int sort : 1;
} settings;

extern settings cfg;

void print_settings(void);
void read_user_settings(int fieldmapping_max);

#endif /* SETTINGS_H */
