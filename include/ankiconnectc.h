#ifndef ANKICONNECTC_H
#define ANKICONNECTC_H

typedef struct {
  char *deck;
  char *notetype;

  size_t num_fields;
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
          char** stringv;
          _Bool boolean;
      } data;
      _Bool ok; // Signalizes if there was an error on not. The error msg is stored in data.string
} retval_s;

/*
 * Returns: The null terminated list of decks in data.stringv
 */
retval_s ac_get_decks(void);

/*
 * Returns: A null terminated list of notetypes in data.stringv
 */
retval_s ac_get_notetypes(void);

/* 
 * Search the Anki database for the entry @entry in the field @field and deck @deck.
 * @deck can be null.
 * The search result will be stored in data.boolen if there was no error.
 */
retval_s ac_search(_Bool include_suspended, char* deck, char* field, char* entry);

/* 
 * Performs a search as in ac_search, but in the Anki gui.
 */
retval_s ac_gui_search(const char* deck, const char* field, const char* entry);

/*
 * Stores the file at @path in the Anki media collection under the name @filename.
 * Doesn't overwrite existing files.
 */
retval_s ac_store_file(char const* filename, char const* path);

/* Add ankicard to anki  */
retval_s ac_addNote(ankicard ac);

#endif
