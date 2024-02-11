#ifndef ANKICONNECTC_H
#define ANKICONNECTC_H

#include <stdbool.h>

typedef struct {
  char *deck;
  char *notetype;

  signed int num_fields; /* If set to -1, fieldnames is expected to be null terminated */
  char **fieldnames;   
  char **fieldentries; 
  char **tags;
} ankicard;

void ankicard_free(ankicard ac);

/*
 * Print the contents of an ankicard 
 */
void ac_print_ankicard(ankicard ac);

typedef struct {
      union {
          char* string;
          char* stringv;
          bool boolean;
      } data;
      bool ok; // Signalizes if there was an error on not. The error msg is stored in data.string
} retval_s;

/*
 * Returns: The null terminated list of decks in data.stringv
 */
retval_s ac_get_decks();

/*
 * Returns: The null terminated list of notetypes in data.stringv
 */
retval_s ac_get_notetypes();

/* 
 * Search the Anki database for the entry @entry in the field @field and deck @deck.
 * @deck can be null.
 * The search result will be stored in data.boolen if there was no error.
 */
retval_s ac_search(bool include_suspended, const char* deck, const char* field, const char* entry);

/* 
 * Performs a search as in ac_search but in the Anki gui.
 */
retval_s ac_gui_search(const char* deck, const char* field, const char* entry);

/*
 * Stores the file at @path in the Anki media collection under the name @filename.
 * Doesn't overwrite existing files.
 */
retval_s ac_store_file(const char* filename, const char* path);

/* Add ankicard to anki  */
retval_s ac_addNote(ankicard ac);

#endif
