#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>
#include <ctype.h>

#include <glib.h>
#include "unishox2.h"

#include "dbwriter.h"
#include "util.h"

#define INITIAL_SIZE 16384

#define ds8(s)     (ds8){ (uint8_t*)s, lengthof(s), .cap = 0 }
typedef struct {
	uint8_t* data;
	ptrdiff_t len;
	ptrdiff_t cap;
} ds8;

typedef struct {
	ds8 dictname;
	ds8 kanji;
	ds8 reading;
	ds8 definition;
} dictentry_s;

ds8
ds8_new(size cap)
{
	return (ds8) { .data = g_malloc(cap), .len = 0, .cap = cap };
}

bool
ds8_equals(ds8 a, ds8 b)
{
	if (a.len != b.len)
		return false;

	for (ptrdiff_t i = 0; i < a.len; i++)
		if (a.data[i] != b.data[i])
			return false;

	return true;
}

void
ds8_append(ds8* str, char c)
{
	if (str->len + 1 > str->cap)
	{
		(str->cap) *= 2;
		str->data = g_realloc(str->data, (size_t)(str->cap));
	}
	(str->data)[(str->len)++] = c;
}

void
ds8_free(ds8* str)
{
	g_free(str->data);
	str->data = NULL;
}

void
ds8_trim(ds8* str)
{
	while (str->len > 0 && isspace(str->data[str->len - 1]))
		str->len--;
}

void
ds8_unescape(ds8* str)
{
	assert(str);
	ptrdiff_t s = 0, e = 0;
	while (e < str->len)
	{
		if (str->data[e] == '\\' && e + 1 < str->len && str->data[e + 1] == 'n')
		{
			str->data[s++] = '\n';
			e += 2;
		}
		else
			str->data[s++] = str->data[e++];
	}
	str->len = s;
}

/* static char */
/* _fgetc(FILE* fp) */
/* { */
/* 	char c = fgetc(fp); */
/* 	putchar(c); */
/* 	return c; */
/* } */

/* #define _assert(expr) \ */
/* 	if (!(expr))             \ */
/* 	{               \ */
/* 		exit(0);      \ */
/* 	} */

static ds8
compress_dictentry(dictentry_s de)
{
	ds8 cstr = {0};
	size data_str_len = de.dictname.len + 1 + de.kanji.len + 1 + de.reading.len + 1 + de.definition.len;

	char data_str[data_str_len];
	char* data_ptr = data_str;

	memcpy(data_ptr, de.dictname.data, de.dictname.len);
	data_ptr += de.dictname.len;
	*data_ptr++ = '\0';

	memcpy(data_ptr, de.kanji.data, de.kanji.len);
	data_ptr += de.kanji.len;
	*data_ptr++ = '\0';

	memcpy(data_ptr, de.reading.data, de.reading.len);
	data_ptr += de.reading.len;
	*data_ptr++ = '\0';

	memcpy(data_ptr, de.definition.data, de.definition.len);

	// TODO: Use string array compression from unishox2 instead
	// TODO: Can overflow
	cstr = ds8_new(data_str_len + 10);
	cstr.len = unishox2_compress(data_str, (int)data_str_len, 
	    UNISHOX_API_OUT_AND_LEN((char*)cstr.data, (int)cstr.len), USX_PSET_FAVOR_SYM);
	return cstr;
}

/* static void */
/* dump_ds8v(int len, ds8 stringv[len]) */
/* { */
/* 	for (int i = 0; i < len; i++) */
/* 		printf("[%i]: %.*s\n", i, (int)stringv[i].len, stringv[i].data); */
/* 	printf("\n"); */
/* } */

static void
add_dictentry(dictentry_s de)
{
	ds8_unescape(&de.definition);
	ds8_trim(&de.definition);

	/* if (de.definition.len == 0) */
	/* 	fprintf(stderr, "Warning: Could not parse a definition for entry with kanji: \"%.*s\" and reading: \"%.*s\". In dictionary: \"%.*s\"\n", */
	/* 		(int)de.kanji.len, de.kanji.data, */
	/* 		(int)de.reading.len, de.reading.data, */
	/* 		(int)de.dictname.len, de.dictname.data); */
	/* else if (de.kanji.len == 0 && de.reading.len == 0) */
	/* 	fprintf(stderr, "Warning: Could not obtain kanji nor reading for entry with definition: \"%.*s\", in dictionary: \"%.*s\"\n", */
	/* 		(int)de.definition.len, de.definition.data, */
	/* 		(int)de.dictname.len, de.dictname.data); */
	/* else */
	/* { */
	if (de.definition.len != 0 && (de.kanji.len != 0 || de.reading.len != 0))
	{
		ds8 compressed = compress_dictentry(de);

		if (*de.kanji.data)
			addtodb((char*)de.kanji.data, de.kanji.len, (char*)compressed.data, compressed.len);
		if (*de.reading.data) // Let the db figure out duplicates
			addtodb((char*)de.reading.data, de.reading.len, (char*)compressed.data, compressed.len);

		ds8_free(&compressed);
	}
}

#define ds8_reset_entries(array)                  \
	for (int i = 0; i < countof(array); i++) \
	array[i].len = 0;

/*
 * Supposes fp points to the starting '"'
 * Stops parsing on last "
 */
static void
read_string_into(ds8* buffer, FILE* fp)
{
	char c = fgetc(fp);
	char prev_c = 0;
	// TODO: Handle escape sequences
	bool escaped_slash = false;
	while (c != EOF)
	{
		if (c == '\\')
			escaped_slash = (prev_c == '\\' && !escaped_slash);
		else if (c == '"' && (prev_c != '\\' || escaped_slash))
			break;

		ds8_append(buffer, c);

		prev_c = c;
		c = fgetc(fp);
	}
	assert(c == '"');
}

static void
read_number_into(ds8* buffer, FILE* fp)
{
	char c;
	for (c = fgetc(fp); isdigit(c); c = fgetc(fp))
		ds8_append(buffer, c);
	ungetc(c, fp);
}

static void
read_next_entry_into(ds8* buffer, FILE* fp, int depth)
{
	char c;
	/* Skip to start */
	for (c = fgetc(fp); c == ':' || isspace(c); c = fgetc(fp))
		;

	if (isdigit(c) || c == '-') // Interpret - as negative sign
	{
		ds8_append(buffer, c);
		read_number_into(buffer, fp);
	}
	else if (c == '"')
		read_string_into(buffer, fp);
	else if (c == '[')
	{       /* read all entries contained in [] into buffer */
		bool stop = false;
		while (!stop)
		{
			read_next_entry_into(buffer, fp, depth+1);

			// Check if this was the last entry
			while ((c = fgetc(fp)))
			{
				if (c == ',')
				{       // Next incoming
					break;         
				}
				else if (c == ']')
				{
					stop = true;
					break;
				}
				assert(isspace(c));
			}
			// Check if this was the last entry
			/* c = next_non_whitespace(fp); */
			/* if (c == ']') */
			/* 	break; */
			/* else */
			/* 	ungetc(c, fp); */
		}
		assert(c == ']');
	}
	else if (c == '{')
	{
		// search for string "content" (if existing)
		// Add entry after it to buffer
		ds8 json_entry = ds8_new(50);
		int cbracket_lvl = 1;

		for (c = fgetc(fp); c != EOF; c = fgetc(fp))
		{
			if (c == '"' && cbracket_lvl == 1)
			{
				read_string_into(&json_entry, fp);
				if (ds8_equals(json_entry, ds8("content")))
					read_next_entry_into(buffer, fp, depth+1);
				json_entry.len = 0;
			}
			else if (c == '{')
				cbracket_lvl++;
			else if (c == '}')
			{
				cbracket_lvl--;
				if (cbracket_lvl <= 0)
					break;
			}
		}
		assert(c == '}');
		if(depth == 3) ds8_append(buffer, '\n');
	}
	else if (c == ',' || c == ']' || c == '}')
		ungetc(c, fp);
	else if (c == 'n')
	{
		// Assume this is null, so skip and leave empty
		while ((c = fgetc(fp)) != ',')
			;
		ungetc(c, fp);
	}
	else
		assert(false);
}

/*
 * Advances the pointer to the next ocurrence of @skip_c
 *
 * Returns: The last read char
 */
static char
skip_to(char skip_c, FILE* fp)
{
	char c;
	for (c = fgetc(fp); c != EOF && c != skip_c; c = fgetc(fp))
		;
	return c;
}

static void
parse_json(ds8 dictname, char* json_file_path)
{
	FILE* fp;
	if (!(fp = fopen(json_file_path, "r")))
	{
		fprintf(stderr, "Error opening file.\n");
		return;
	}

	ds8 entry_buffer[] = {
		(ds8){ .data = g_malloc(INITIAL_SIZE), .cap = INITIAL_SIZE },
		(ds8){ .data = g_malloc(INITIAL_SIZE), .cap = INITIAL_SIZE },
		(ds8){ .data = g_malloc(INITIAL_SIZE), .cap = INITIAL_SIZE },
		(ds8){ .data = g_malloc(INITIAL_SIZE), .cap = INITIAL_SIZE },
		(ds8){ .data = g_malloc(INITIAL_SIZE), .cap = INITIAL_SIZE },
		(ds8){ .data = g_malloc(INITIAL_SIZE), .cap = INITIAL_SIZE }
	};

	skip_to('[', fp);
	while (true)
	{
		if (skip_to('[', fp) == EOF)
			break;

		for (int i = 0; i < countof(entry_buffer); i++)
		{
			read_next_entry_into(&entry_buffer[i], fp, 0);
			if (i != countof(entry_buffer) - 1)
				skip_to(',', fp);
		}

		add_dictentry((dictentry_s) {
			.dictname = dictname,
			.kanji = entry_buffer[0],
			.reading = entry_buffer[1],
			.definition = entry_buffer[5]
		});
		ds8_reset_entries(entry_buffer);

		skip_to(']', fp);
	}

	for (int i = 0; i < countof(entry_buffer); i++)
		ds8_free(&entry_buffer[i]);
	fclose(fp);
}

ds8
extract_dictname(char *directory_path)
{
	g_autofree char* index_file = g_build_filename(directory_path, "index.json", NULL);
	g_autofree char* index_contents = NULL;
	g_file_get_contents(index_file, &index_contents, NULL, NULL);

	const char* search_string = "\"title\"";
	const char *title_start = strstr(index_contents, search_string);
	if (!title_start)
		return (ds8){ 0 }
	;
	title_start += strlen(search_string);
	title_start = strstr(title_start, "\"") + 1; // No error handling

	const char *title_end = strstr(title_start, "\"");

	return (ds8){
		       .data = (u8*)g_strndup(title_start, title_end - title_start),
		       .len = title_end - title_start,
		       .cap = title_end - title_start
	};
}

static void
add_folder(char *directory_path)
{
	ds8 dictname = extract_dictname(directory_path);
	printf("Processing dictionary: %.*s\n", (int)dictname.len, dictname.data);

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
		    && strncmp(entry->d_name, "term", lengthof("term")) == 0
		    && strncmp(entry->d_name + len - 5, ".json", lengthof(".json")) == 0)
		{
			char filepath[1024];
			snprintf(filepath, sizeof(filepath), "%s/%s", directory_path, entry->d_name);
			parse_json(dictname, filepath);
		}
	}
	closedir(dir);

	ds8_free(&dictname);
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


	opendb(db_path);
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
			printf("Unpacking \"%s\"\n", entry->d_name);
			printf_cmd_sync("unzip -qq '%s' -d tmp", entry->d_name);
			add_folder("tmp");
			printf_cmd_sync("rm -rf ./tmp");
		}
	}
	closedir(dir);
	closedb();


	g_autofree char* lock_file = g_build_filename(db_path, "lock.mdb", NULL);
	remove(lock_file);
	//FIXME
	remove("/tmp/data.mdb");
	remove("/tmp/lock.mdb");
}
