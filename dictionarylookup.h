#include "dictionary.h"


void open_database();
void close_database();

/*
 * Looks up @word in the users dictionaries and adds 
 * the results to @dict. 
 *
 * @word is copied if used in a dictionary entry.
 */
void add_word_to_dict(dictionary *dict, char *word);

size_t longest_entry_length();
