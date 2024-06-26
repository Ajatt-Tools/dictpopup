#ifndef SETTINGS_H
#define SETTINGS_H
#include "utils/util.h"
#include <stdbool.h>

typedef struct {
    struct {
        char *dbpth;
        bool sort;
        char **dictSortOrder;
        bool nukeWhitespaceLookup;
        bool mecab;
        bool substringSearch;
    } general;

    struct {
        bool enabled;
        char *deck;
        char *notetype;
        bool copySentence;
        bool nukeWhitespaceSentence;
        char **fieldnames;
        size_t numFields;
        u32 *fieldMapping;
        bool checkExisting;
        char *searchField;
    } anki;

    struct {
        u32 width;
        u32 height;
        u32 margin;
    } popup;

    struct {
        bool displayButton;
        bool onStart;
        char *dirPath;
    } pron;
} Config;

extern Config cfg;

void print_settings(void);
void read_user_settings(int fieldmapping_max);

#endif /* SETTINGS_H */
