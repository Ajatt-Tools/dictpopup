#include <glib.h>


/*
   Fills the deinflections array with possible deinflections.
   Contains all intermediate steps, e.g. してしまった -> してしまう, して, する
   Returns an error description on error and NULL else
*/
const char *deinflect(GPtrArray *deinflections, char *wordSTR);
