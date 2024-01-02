#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "dictionary.h"

/*
 * Execute sdcv to look up the word.
 * Returns: The output as a string in json format. Needs to be freed.
 */
char *
lookup(char *word)
{
	char *std_out = NULL;
	char *sdcv_cmd[] = { "sdcv", "-nej", "--utf8-output", word, NULL };
	g_spawn_sync(NULL, sdcv_cmd, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &std_out, NULL, NULL, NULL);

	Stopif(!std_out, exit(1), "Error executing sdcv. Is it installed?");

	return std_out;
}

int
is_empty_json(char *json)
{
	return !json || !*json ? 1 : (strncmp(json, "[]", 2) == 0);
}

/*
 * Adds all dictionary entries from json string to dict.
 * Changes json string in process
 * json can be NULL.
 */
void
add_from_json(dictionary* dict, char *lookupstr, char *json)
{
	if (is_empty_json(json))
		return;

	str_repl_by_char(json, "\\n", '\n');
	const char *keywords[] = { "\"dict\"", "\"word\"", "\"definition\"" };
	char *entries[3] = { NULL };

	char *start;
	for (char* i = json + 1; *i ; i++)
	{
		for (int k = 0; k < 3; k++)
		{
			if (strncmp(i, keywords[k], strlen(keywords[k])) == 0)
			{       /* TODO: string bounds checking */
				i += strlen(keywords[k]);
				while (*i != '"')
					i++;

				while (*(++i) == '\n') // Skip leading newlines
					;
				start = i;

				while (*i != '"' || *(i - 1) == '\\') // not escaped
					i++;

				*i++ = '\0';

				if (entries[k])
					g_warning("WARNING: Overwriting previous entry. \
					    Expects to see dict, word and definition before next entry.");
				entries[k] = start;

				if (entries[0] && entries[1] && entries[2])
				{
					dictionary_copy_add(dict,
							    (dictentry)
							    { .dictname = entries[0],
							      .word = entries[1],
							      .definition = entries[2],
							      .lookup = lookupstr });
					for (int l = 0; l < 3; l++)
						entries[l] = NULL;
				}
				break;
			}
		}
	}
}

void
add_word_to_dict(dictionary *dict, char *word)
{
	char *sdcv_output = lookup(word);
	add_from_json(dict, word, sdcv_output);
	free(sdcv_output);
}
