#include "readsettings.h"
#include "dictionary.h"

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
dictentry* popup();

/*
 * Used to pass the dictionary data once it is ready
 */
void dictionary_data_done(dictionary* passed_dict);
