#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "dictionary.h"
#include "util.h"

/* dictentry * */
/* dictentry_new(char *dictname, char *dictword, char *definition, char *lookup) */
/* { */
/* 	dictentry *de = malloc(sizeof(dictentry)); */

/* 	*de = (dictentry) { */
/* 		.dictname = dictname ? strdup(dictname) : NULL, */
/* 		.kanji = dictword ? strdup(dictword) : NULL, */
/* 		.definition = definition ? strdup(definition) : NULL, */
/* 		.lookup = lookup ? strdup(lookup) : NULL */
/* 	}; */

/* 	return de; */
/* } */

void
dictentry_print(dictentry *de)
{
    printf("dictname: %s\nkanji: %s\nreading: %s\ndefinition: %s\n", de->dictname, de->kanji, de->reading, de->definition);
}

void
dictionary_print(dictionary* dict)
{
  for (int i = 0; i < dict->len; i++)
	dictentry_print(dictentry_at_index(dict, i));
}

void
dictentry_free(void *ptr)
{
	dictentry *de = ptr;
	g_free(de->dictname);
	g_free(de->kanji);
	g_free(de->reading);
	g_free(de->definition);
	g_free(de);
}

dictionary*
dictionary_new()
{
	return g_ptr_array_new_with_free_func(dictentry_free);
}

void
dictionary_copy_add(dictionary* dict, dictentry de)
{
	dictentry *de_copy = g_new(dictentry, 1);

	*de_copy = (dictentry) { 
		.dictname = g_strdup(de.dictname),
		.kanji = g_strdup(de.kanji),
		.reading = g_strdup(de.reading),
		.definition = g_strdup(de.definition),
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
