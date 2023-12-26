#include "readsettings.h"


/*
 * @dictionary: The dictionary to be displayed
 * @definition: Return location for the chosen definition by the user
 * 		that should be added to the Anki card.
 * @de_num: Return location for the index of the chosen dictionary entry by the user.
 * 
 * Displays a popup with the dictionary content.
 * 
 * Returns: TRUE if an Anki card should be made and FALSE otherwise.
 */
int popup(GPtrArray *dictionary, char** definition, size_t *de_num, settings *cfg);

/*
 * Signals that the dictionary data is currently in use.
 */
void lock_dictionary_data();

/*
 * Signals that the dictionary data isn't in use anymore.
 */
void dictionary_data_done(settings *cfg);
