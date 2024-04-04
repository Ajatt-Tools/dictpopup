#ifndef SETTINGS_H
#define SETTINGS_H
#include <stdbool.h>
#include <stddef.h>
#include "util.h"

typedef struct {
    struct {
	char* dbpth;
	bool sort;
	char** dictSortOrder;
	bool nukeWhitespaceLookup;
	bool mecab;
	bool substringSearch;
    } general;

    struct {
	bool enabled;
	char* deck;
	char* notetype;
	bool copySentence;
	bool nukeWhitespaceSentence;
	char** fieldnames;
	size_t numFields;
	u32* fieldMapping;
	bool checkExisting;
	char* searchField;
    } anki;

    struct {
	u32 width;
	u32 height;
	u32 margin;
    } popup;

    struct {
	bool displayButton;
	bool onStart;
	char* dirPath;
    } pron;

    struct {
	unsigned int debug : 1;
    } args;
} settings;

extern settings cfg;

void print_settings(void);
void read_user_settings(int fieldmapping_max);
int parse_cmd_line_opts(int argc, char** argv);

#endif /* SETTINGS_H */
