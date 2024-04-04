#include "util.h"

void open_database(void);
void close_database(void);

/*/1* */
/* * Looks up @word in the database and adds */ 
/* * the results to @dict. */ 
/* * */
/* * @word is copied if used in a dictionary entry. */
/* *1/ */
/*void add_word_to_dict(dictentry* dict[static 1], s8 word); */

s8 db_getdata(u32 id);
u32* db_getids(s8 key, size_t len[static 1]);
int db_getfreq(s8 key);
