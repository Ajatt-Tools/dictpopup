#include <stdlib.h>
#include <string.h>
#include <mecab.h>
#include <glib.h>

char*
kata2hira(const char* katakana_in)
{
	unsigned char* hiragana_out = (unsigned char*)strdup(katakana_in);
	unsigned char* h = hiragana_out;
	while (*h)
	{
		/* Check that this is within the katakana block from E3 82 A0
		   to E3 83 BF. */
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
			h += 3;
		}
		else
		{
			h++;
		}
	}

	return (char *)hiragana_out;
}

char*
kanji2hira(char *input)
{
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

/* char* */
/* hira2kata (const char *hira_input) */
/* { */
/*     unsigned char* output = (unsigned char*) strdup(hira_input); */
/*     unsigned char* k = output; */
/*     while (*k) { */
/*         /1* Check that this is within the hiragana block *1/ */
/*         if (k[0] == 0xe3 && (k[1] == 0x81 || k[1] == 0x82) && k[2] != '\0') { */
/*             /1* Check that this is within the range of katakana which */
/*                can be converted into hiragana. *1/ */
/*             if ((k[1] == 0x81 && (k[2] >= 0x81 && k[2] <= 0xbf)) || */
/*                 (k[1] == 0x82 && (k[2] >= 0x80 && k[2] <= 0x96))) { */
/*                 /1* Byte conversion from katakana to hiragana. *1/ */
/* 		/1* if (h[1] >= 0x82) *1/ */
/*                 if (k[1] == 0x82 || k[2] <= 0x9f) { */
/*                     k[1] = k[1] + 1; */
/*                     k[2] += 0x20; */
/*                 } */
/*                 else { */
/*                     k[1] = k[1] + 2; */
/*                     k[2] -= 0x20; */
/*                 } */
/*             } */
/*             k += 3; */
/*         } */
/*         else { */
/*             k++; */
/*         } */
/*     } */

/*     return (char *)output; */
/* } */
