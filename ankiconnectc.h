
typedef size_t (*ResponseFunction)(char *ptr, size_t size, size_t nmemb, void *userdata);

typedef struct {
  char *deck;
  char *notetype;

  int num_fields;
  char **fieldnames;
  char **fieldentries;
  char *tags; //space separated list
} ankicard;

/* Create a new anki card. Needs freeing. */
ankicard* new_ankicard();

/* Perform a database search. Result is passed to the response function. */
const char *search_query(const char *query, ResponseFunction respf);

const char *action_query(const char *action, const char *query, ResponseFunction respf);

/* Add ankicard to anki  */
const char *addNote(ankicard ac);

/* Print contents of an anki card */
void print_anki_card(ankicard ac);
