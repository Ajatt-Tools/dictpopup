#include <stdlib.h>
#include <string.h>
#include <mecab.h>

#include <glib.h>

static char*
kata2hira(const char* katakana_in)
{
	if (!katakana_in)
	  return NULL;

	unsigned char* hiragana_out = (unsigned char*) g_strdup(katakana_in);
	unsigned char* h = hiragana_out;

	while (*h)
	{
		/* Check that this is within the katakana block from E3 82 A0 to E3 83 BF. */
		if (h[0] == 0xe3 && (h[1] == 0x82 || h[1] == 0x83) && h[2] != '\0')
		{
			/* Check that this is within the range of katakana which
			   can be converted into hiragana. */
			if ((h[1] == 0x82 && h[2] >= 0xa1) ||
			    (h[1] == 0x83 && h[2] <= 0xb6) ||
			    (h[1] == 0x83 && (h[2] == 0xbd || h[2] == 0xbe)))
			{
				/* Byte conversion from katakana to hiragana. */
				if (h[2] >= 0xa0)
				{
					h[1] -= 1;
					h[2] -= 0x20;
				}
				else
				{
					h[1] = h[1] - 2;
					h[2] += 0x20;
				}
			}
			h = (unsigned char*) g_utf8_next_char(h);
		}
		else
			h++;
	}

	return (char *)hiragana_out;
}

char*
kanji2hira(char *input)
{
	// TODO: Write this properly
	GString* output = g_string_sized_new(strlen(input) + 2);

	mecab_t *mecab = mecab_new2("");
	const mecab_node_t *node = mecab_sparse_tonode(mecab, input);

	char kata_reading[64];

	for (; node; node = node->next)
	{
		if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE)
			continue;

		sscanf(node->feature, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%[^,],", kata_reading);
		if (strlen(kata_reading) == 0)
		{
			strcpy(kata_reading, node->surface);
			kata_reading[node->length] = '\0';
		}
		char* reading_hira = kata2hira(kata_reading);
		g_string_append(output, reading_hira);
		free(reading_hira);
	}

	mecab_destroy(mecab);
	return g_string_free_and_steal(output);
}
