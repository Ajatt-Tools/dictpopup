#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>

#include <glib.h>
#include "unishox2.h"

#include "db_writer.h"
#include "util.h"
#include "dictionary.h"

void
compress_entries(dictentry de, char** compressed, size_t *clen)
{
	unsigned int dictname_len = strlen(de.dictname);
	unsigned int kanji_len = strlen(de.kanji);
	unsigned int reading_len = strlen(de.reading);
	unsigned int definition_len = strlen(de.definition);
	unsigned int data_str_len = dictname_len + 1 + kanji_len + 1 + reading_len + 1 + definition_len;

	char data_str[data_str_len];
	char* data_ptr = data_str;

	memcpy(data_ptr, de.dictname, dictname_len);
	data_ptr += dictname_len;
	*data_ptr++ = '\0';

	memcpy(data_ptr, de.kanji, kanji_len);
	data_ptr += kanji_len;
	*data_ptr++ = '\0';

	memcpy(data_ptr, de.reading, reading_len);
	data_ptr += reading_len;
	*data_ptr++ = '\0';

	memcpy(data_ptr, de.definition, definition_len);

	// TODO: Use string array compression from unishox2 instead
	unsigned int csize = data_str_len;
	*compressed = malloc(data_str_len + 5);
	do{
		csize += 10;
		*compressed = realloc(*compressed, csize);
		*clen = unishox2_compress(data_str, data_str_len, UNISHOX_API_OUT_AND_LEN(*compressed, csize), USX_PSET_FAVOR_SYM);
	} while (*clen > csize);
}

void
write_entries(char *dictname, char **entries)
{
	str_repl_by_char(entries[2], "\\n", '\n');
	g_strchomp(entries[2]); // remove trailing whitespace. FIXME: Maybe don't include it そもそも

	dictentry de = (dictentry) {
		.dictname = dictname,
		.kanji = entries[0],
		.reading = entries[1],
		.definition = entries[2]
	};


	/* if (!*de.definition) */
	/* 	fprintf(stderr, "Error parsing entry with kanji: %s and reading: %s.\n", entries[0], entries[1]); */
	/* else */
	if (*de.definition)
	{
		char* compressed_entry;
		size_t compressed_len;
		compress_entries(de, &compressed_entry, &compressed_len);

		if (*de.kanji)
			add_to_db(de.kanji, strlen(de.kanji), compressed_entry, compressed_len);
		if (*de.reading) // Let the db figure out duplicates
			add_to_db(de.reading, strlen(de.reading), compressed_entry, compressed_len);

		free(compressed_entry);
	}
}

void
reset_entries(char** entries)
{
	for (int i = 0; i < 3; i++)
		*entries[i] = '\0';
}

void
str_append(char *str, char c)
{
	int len = strlen(str);
	str[len] = c;
	str[len + 1] = '\0';
}

void
read_string(FILE* fp, char* buffer, unsigned int buffer_len)
{
	char c = 0, prev_c = 0;
	char* ptr = buffer;
	unsigned int cur_len = 0;

	while ((c = fgetc(fp)) != EOF && cur_len < buffer_len - 1)
	{
		if (c == '"' && prev_c != '\\')
			break;

		*ptr++ = c;
		cur_len++;
		prev_c = c;
	}

	buffer[cur_len] = '\0';
}

void
skip_whitespace(FILE* fp)
{
	char c;
	do{
		c = fgetc(fp);
	} while (c == ' ' || c == '\n' || c == '\t');
	ungetc(c, fp);
}

/*
 * Returns: true if next entry is a string, false otherwise or on parsing error
 */
bool
skip_to_next_string(FILE* fp)
{
	char c;

	skip_whitespace(fp);
	if ((c = fgetc(fp)) != ':')
	{
		ungetc(c, fp);
		return false;
	}
	skip_whitespace(fp);
	if ((c = fgetc(fp)) != '"')
	{
		ungetc(c, fp);
		return false;
	}

	return true;
}


void
write_json(char* dictname, char* json_file_path)
{
	FILE* fp;
	if (!(fp = fopen(json_file_path, "r")))
	{
		fprintf(stderr, "Error opening file.\n");
		return;
	}

	char *dictentries[3];
	for (int i = 0; i < 3; i++)
	{
		dictentries[i] = g_malloc(20000); //FIXME: ridiculous of course
		dictentries[i][0] = '\0';
	}

	char prev_c = 0, c = 0;

	bool in_string = false, in_structured_content = false;
	unsigned int sbracket_lvl = 0, cbracket_lvl = 0;

	unsigned int entry_nr = 0;
	char *str_ptr = dictentries[0];

	/* enum { LIST_NONE, LIST_STYLE_CIRCLE, LIST_STYLE_NUMBER }; */
	/* unsigned int list_style = LIST_NONE; */
	/* unsigned int list_number = 0; */

	while ((c = fgetc(fp)) != EOF)
	{
		if (c == '"' && prev_c != '\\')
		{
			if (
				(entry_nr == 2 && sbracket_lvl < 3) // Don't read anything in between reading and content
				||
				entry_nr >= 3 // Don't read anything after content
				)
				continue;


			in_string = !in_string;

			if (!in_string && entry_nr < 2)
			{
				*str_ptr = '\0';
				entry_nr++;
				str_ptr = dictentries[entry_nr];
			}

			if (in_string && in_structured_content)
			{
				in_string = false;

				char buffer[20000]; // Can (falsely) contain dict entries
				read_string(fp, buffer, sizeof(buffer));

				// Only read in strings coming directly after "content"
				if (strcmp(buffer, "content") == 0 && skip_to_next_string(fp))
					in_string = true;
				/* else if (strcmp(buffer, listStyleType) == 0) */
				/* { */
				/* 	if (!skip_to_next_string()) */
				/* 		continue; */

				/* 	read_string(fp, buffer, sizeof(buffer)); */
				/* } */
			}

			continue;
		}

		if (in_string)
			*str_ptr++ = c;
		else
		{
			if (c == '[')
			{
				sbracket_lvl++;
				/* printf("prev: %c, sbracket_lvl: %i\n", prev_c, sbracket_lvl); */
			}
			else if (c == ']')
			{
				sbracket_lvl--;
				/* printf("prev: %c, sbracket_lvl: %i\n", prev_c, sbracket_lvl); */

				if (sbracket_lvl == 2)
				{
					*str_ptr = '\0';
					entry_nr++;
				}
				else if (sbracket_lvl == 1)
				{
					write_entries(dictname, dictentries);
					reset_entries(dictentries);

					entry_nr = 0;
					str_ptr = dictentries[entry_nr];
				}
				else if (sbracket_lvl == 0)
					break;         /* End of file or bad json. */
			}
			else if (c == '{')
			{
				cbracket_lvl++;

				in_structured_content = true;
			}
			else if (c == '}')
			{
				cbracket_lvl--;

				if (cbracket_lvl == 1)
					*str_ptr++ = '\n';
				else if (cbracket_lvl == 0)
					in_structured_content = false;
			}
		}


		prev_c = c;
	}


	for (int i = 0; i < 3; i++)
		free(dictentries[i]);
	fclose(fp);
}

char*
extract_dictname(char *directory_path)
{
	g_autofree char* index_file = g_build_filename(directory_path, "index.json", NULL);
	g_autofree char* index_contents = NULL;
	g_file_get_contents(index_file, &index_contents, NULL, NULL);

	const char* search_string = "\"title\"";
	const char *title_start = strstr(index_contents, search_string);
	if (!title_start)
		return g_strdup("");
	title_start += strlen(search_string);
	title_start = strstr(title_start, "\"") + 1; // No error handling

	const char *title_end = strstr(title_start, "\"");

	return g_strndup(title_start, title_end - title_start);
}

void
write_folder(char *directory_path)
{
	g_autofree char *dictname = extract_dictname(directory_path);
	printf("Processing dictionary: %s\n", dictname);

	DIR *dir = opendir(directory_path);
	if (dir == NULL)
	{
		perror("Error opening directory");
		exit(1);
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		size_t len = strlen(entry->d_name);
		if (len > 5
		    && strcmp(entry->d_name + len - 5, ".json") == 0
		    && strcmp(entry->d_name, "index.json") != 0)
		{
			char filepath[1024];
			snprintf(filepath, sizeof(filepath), "%s/%s", directory_path, entry->d_name);
			write_json(dictname, filepath);
		}
	}
	closedir(dir);
}


int
main(int argc, char * argv[])
{
	g_autofree char* db_path = NULL;
	if (argc > 1)
		db_path = argv[1];
	else
	{
		const char* data_dir = g_get_user_data_dir();
		db_path = g_build_filename(data_dir, "dictpopup", NULL);
		printf("Using default path: %s\n", db_path);
	}

	//TODO: Ask for confirmation first
	char* data_fp = g_build_filename(db_path, "data.mdb", NULL);
	remove(data_fp);
	g_free(data_fp);


	open_db(db_path);
	DIR *dir = opendir(".");
	if (dir == NULL)
	{
		perror("Error opening current directory");
		exit(1);
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		size_t len = strlen(entry->d_name);
		if (len > 4 && strcmp(entry->d_name + len - 4, ".zip") == 0)
		{
			printf_cmd_sync("unzip -qq '%s' -d tmp", entry->d_name);
			write_folder("tmp");
			printf_cmd_sync("rm -rf ./tmp");
		}
	}
	closedir(dir);
	close_db();


	g_autofree char* lock_file = g_build_filename(db_path, "lock.mdb", NULL);
	remove(lock_file);
	//FIXME
	remove("/tmp/data.mdb");
	remove("/tmp/lock.mdb");
}
