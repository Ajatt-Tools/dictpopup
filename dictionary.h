#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <glib.h>

typedef struct {
  char *dictname;
  char *kanji;
  char *reading;
  char *definition;
} dictentry;

/* 
 * This provides type checking and more readable syntax.
 */
typedef GPtrArray dictionary;

/*
 * Returns a dictionary entry with the given entries and null on error.
 * Result needs to be freed with dictentry_free
 */
/* dictentry* dictentry_new(char *dictname, char *dictword, char *definition, char *lookup); */
/*
 * Frees a dictentry created with dictentry_new
 */
void dictentry_free(void *ptr);

/*
 * Returns: A newly allocated dictionary.
 */
dictionary* dictionary_new();

/*
 * Adds the pointer @de to the dictionary pointed to by @dict.
 */
/* void dictionary_add(dictionary *dict, dictentry *de); */

/*
 * Adds a copy of @de to the dictionary pointed to by @dict
 */
void dictionary_copy_add(dictionary *dict, dictentry de);

/*
 * Frees @dict
 */
void dictionary_free(dictionary* dict);

/*
 * Returns a pointer to the dictentry at the given index.
 */
dictentry* dictentry_at_index(dictionary* dict, size_t index);

void dictionary_print(dictionary *dict);

#endif /* DICTIONARY_H */
