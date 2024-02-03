#include <stdbool.h>


/* 
 * @exists contains = 1 on successfull search and 0 otherwise 
 */
typedef void (*SearchResponseFunction)(bool exists);

/*
 * @err: Contains an error description on error and NULL otherwise.
 */
typedef void (*AddResponseFunction)(const char *err);

typedef struct {
  char *deck;
  char *notetype;

  int num_fields;	/* If set to -1, fieldnames is expected to be null terminated */
  char **fieldnames;   
  char **fieldentries; 
  char **tags; //space separated list
} ankicard;

char** ac_get_decks();

char** ac_get_notetypes();

/* 
 * Create an anki card with entries initialized to NULL and num_fields set to -1. Needs to be freed. 
 */
ankicard* ankicard_new();
void ankicard_free(ankicard* ac, bool free_contents);

/* 
 * Search the Anki database for the entry @entry in the field @field and deck @deck.
 * @deck can be null.
 * The search result is passed to @user_function as an argument.
 */
const char* ac_search(bool include_suspended, const char* deck, const char* field, const char* entry, SearchResponseFunction user_function);

/* 
 * Performs a search as in ac_search but in the Anki gui.
 */
const char* ac_gui_search(const char* deck, const char* field, const char* entry);

/*
 * Sends a request with action @action and a printf style formated query string.
 */
/* const char * ac_action_printf_query(const char *action, char const *fmt, ...); */

/*
 * Stores the file at @path in the Anki media collection under the name @filename.
 * Doesn't overwrite existing files.
 */
const char* ac_store_file(const char* filename, const char* path);

/* Add ankicard to anki  */
const char* ac_addNote(ankicard *ac, AddResponseFunction user_func);

/* Print contents of an anki card */
void ac_print_ankicard(ankicard *ac);
