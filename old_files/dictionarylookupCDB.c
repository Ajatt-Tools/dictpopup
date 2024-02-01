#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <cdb.h>

#include <unistd.h> // close

#include "dictionary.h"
#include "unishox2.h"

struct cdb cdb;
int fd;


void
open_database()
{
	char *dbname = "/home/daniel/.local/share/dictpopup/cdbfile";
	fd = open(dbname, O_RDONLY);
	if (fd < 0 || cdb_init(&cdb, fd) != 0)
	{
		fprintf(stderr, "Error: %s\n", strerror(errno));
		abort();
	}
}

void
close_database()
{
	if (close(fd) == -1)
	{
		perror("Error closing file");
		exit(1);
	}
}

void
longest_entry_length()
{
    // TODO: Implement
}

void
add_de_from_db_lookup(dictionary *dict, char *db_lookup)
{
	char *ptr_buffer[4];

	int len = 0;
	for (int i = 0; i < 4; i++)
	{
		len = strlen(db_lookup);
		ptr_buffer[i] = db_lookup;
		db_lookup += len + 1;
	}

	dictionary_copy_add(dict, (const dictentry) { .dictname = ptr_buffer[0], .kanji = ptr_buffer[1], .reading = ptr_buffer[2], .definition = ptr_buffer[3] });
}

void
add_word_to_dict(dictionary *dict, char *word)
{
	printf("Looking up: %s\n", word);

	char* key = word;
	unsigned klen = strlen(key);
	struct cdb_find cdbf;
	cdb_findinit(&cdbf, &cdb, key, klen);

	while (cdb_findnext(&cdbf) > 0)
	{
		unsigned vpos = cdb_datapos(&cdb);
		unsigned vlen = cdb_datalen(&cdb);
		char val[vlen];
		cdb_read(&cdb, val, vlen, vpos);

		/* FIXME: This is arbitrary and probably too large.
		 * Need to lookup the proper way of doing this.
		 */
		char uncompressed[5000]; 
		size_t len = unishox2_decompress_simple(val, vlen, uncompressed);
		uncompressed[len] = '\0';
		add_de_from_db_lookup(dict, uncompressed);
	}
}
