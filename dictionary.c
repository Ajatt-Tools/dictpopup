#include <stdlib.h>
#include <glib.h>
#include "dictionary.h"

dictentry *
dictentry_new(char *dictname, char *dictword, char *definition, char *lookup)
{
	dictentry *de = malloc(sizeof(dictentry));

	*de = (dictentry) {
		.dictname = dictname ? strdup(dictname) : NULL,
		.word = dictword ? strdup(dictword) : NULL,
		.definition = definition ? strdup(definition) : NULL,
		.lookup = lookup ? strdup(lookup) : NULL
	};

	return de;
}

void
dictentry_free(void *ptr)
{
	dictentry *de = ptr;
	free(de->dictname);
	free(de->word);
	free(de->definition);
	free(de->lookup);
	free(de);
}

dictionary*
dictionary_new()
{
	return g_ptr_array_new_with_free_func(dictentry_free);
}

/* void */
/* dictionary_add(dictionary *dict, dictentry *de) */
/* { */
/* 	g_ptr_array_add(dict, de); */
/* } */

void
dictionary_copy_add(dictionary *dict, const dictentry de)
{
	dictentry *de_copy = malloc(sizeof(dictentry));
	*de_copy = (dictentry) {
		.dictname = strdup(de.dictname),
		.word = strdup(de.word),
		.definition = strdup(de.definition),
		.lookup = strdup(de.lookup)
	};

	g_ptr_array_add(dict, de_copy);
}

void
dictionary_free(dictionary* dict)
{
	g_ptr_array_free(dict, TRUE);
}

dictentry*
dictentry_at_index(dictionary *dict, size_t index)
{
	return (dictentry*)g_ptr_array_index(dict, index);
}
