#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include <dirent.h>
#include <zip.h>
#include <glib.h>
/* #include "unishox2.h" */

#include "dbwriter.h"
#include "util.h"

typedef struct {
	u8* ptr;
} Parser;

#define ds8(s)  (ds8){ (u8*)s, lengthof(s), 0 }
typedef struct {
	uint8_t* s;
	ptrdiff_t len;
	ptrdiff_t cap;
} ds8;

typedef struct {
	s8 dictname;
	s8 kanji;
	s8 reading;
	s8 definition;
} s8dictentry;

ds8
ds8_new(size cap)
{
	return (ds8) { .s = _malloc(cap), .len = 0, .cap = cap };
}

bool
ds8_equals(ds8 a, ds8 b)
{
	if (a.len != b.len)
		return false;

	for (ptrdiff_t i = 0; i < a.len; i++)
		if (a.s[i] != b.s[i])
			return false;

	return true;
}

void
ds8_appendc(ds8* str, int c)
{
	if (str->len + 1 > str->cap)
		str->s = g_realloc(str->s, (size_t)(str->cap *= 2));

	(str->s)[(str->len)++] = (u8)c;
}

void
ds8_appendstr(ds8* a, ds8 b)
{
	assert(a);
	assert(b.len >= 0);

	if (a->len + b.len > a->cap)
	{
		a->cap = a->len + b.len;
		a->s = g_realloc(a->s, (size_t)(a->cap));
	}

	u8copy(a->s + a->len, b.s, b.len);
	a->len += b.len;
}

void
ds8free(ds8* str)
{
	free(str->s);
	str->s = NULL;
}

bool
_isspace(u8 c)
{
	return(c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

bool
_isdigit(u8 c)
{
	return c >= '0' && c <= '9';
}

s8
s8trim(s8 str)
{
	while (str.len && _isspace(str.s[str.len - 1]))
		str.len--;
	while (str.len && _isspace(*str.s))
	{
		str.s++;
		str.len--;
	}

	return str;
}


#define INITIAL_SIZE 1 << 17
ds8 entry_buffer[6] = { 0 };

// Debug code
/* static char */
/* _fgetc(FILE* fp) */
/* { */
/* 	int c = fgetc(fp); */
/* 	putchar(c); */
/* 	return c; */
/* } */

/* #define _assert(expr) \ */
/* 	if (!(expr))             \ */
/* 	{               \ */
/* 		exit(0);      \ */
/* 	} */

/* static void */
/* dump_ds8v(int len, ds8 stringv[len]) */
/* { */
/* 	for (int i = 0; i < len; i++) */
/* 		printf("[%i]: %.*s\n", i, (int)stringv[i].len, stringv[i].s); */
/* 	printf("\n"); */
/* } */

/* static ds8 */
/* compress_dictentry(dictentry_s de) */
/* { */
/* 	// TODO: Use string array compression from unishox2 instead */

/* 	ds8 cstr = ds8_new(data_str_len + 5); */
/* 	cstr.len = unishox2_compress(data_str, (int)data_str_len, */
/* 				     UNISHOX_API_OUT_AND_LEN((char*)cstr.s, (int)cstr.cap), USX_PSET_FAVOR_SYM); */
/* 	while (cstr.len > cstr.cap) */
/* 	{ */
/* 		cstr.cap += 20; */
/* 		cstr.s = g_realloc(cstr.s, (gsize)(cstr.cap)); */
/* 		cstr.len = unishox2_compress(data_str, (int)data_str_len, */
/* 					     UNISHOX_API_OUT_AND_LEN((char*)cstr.s, (int)cstr.cap), USX_PSET_FAVOR_SYM); */
/* 	} */
/* 	return cstr; */
/* } */

static void
add_dictentry(s8dictentry de)
{
	de.definition = s8unescape(s8trim(de.definition));

	if (de.definition.len == 0)
	{
		fprintf(stderr, "Warning: Could not parse a definition for entry with kanji: \"%.*s\" and reading: \"%.*s\". In dictionary: \"%.*s\"\n",
			(int)de.kanji.len, de.kanji.s,
			(int)de.reading.len, de.reading.s,
			(int)de.dictname.len, de.dictname.s);
	}
	else if (de.kanji.len == 0 && de.reading.len == 0)
	{
		fprintf(stderr, "Warning: Could not obtain kanji nor reading for entry with definition: \"%.*s\", in dictionary: \"%.*s\"\n",
			(int)de.definition.len, de.definition.s,
			(int)de.dictname.len, de.dictname.s);
	}
	else
	{
		s8 datastr = {0};
		datastr.len = de.dictname.len + 1 + de.kanji.len + 1 + de.reading.len + 1 + de.definition.len + 1;
		datastr.s = alloca(datastr.len);

		s8 end = s8copy(datastr, de.dictname);
		end = s8copy(end, s8("\0"));
		end = s8copy(end, de.kanji);
		end = s8copy(end, s8("\0"));
		end = s8copy(end, de.reading);
		end = s8copy(end, s8("\0"));
		end = s8copy(end, de.definition);
		s8copy(end, s8("\0"));

		if (de.kanji.len > 0)
			addtodb(de.kanji.s, de.kanji.len, datastr.s, datastr.len);
		if (de.reading.len > 0) // Let the db figure out duplicates
			addtodb(de.reading.s, de.reading.len, datastr.s, datastr.len);
	}
}

#define ds8_reset_entries(array)                 \
	for (int i = 0; i < countof(array); i++) \
	      array[i].len = 0;

/*
 * Supposes p points to the starting '"'
 * Stops parsing on last "
 */
static void
read_string_into(ds8* buffer, Parser* p)
{
	bool prev_slash = false;
	while (*(++p->ptr))
	{
		if (*p->ptr == '"' && !prev_slash)
			break;

		if (prev_slash && *p->ptr == 'n')
			buffer->s[buffer->len - 1] = '\n';
		else
			ds8_appendc(buffer, *p->ptr);

		prev_slash = (*p->ptr == '\\' && !prev_slash); // Don't count escaped slashes
	}

	assert(*p->ptr == '"');
}

static void
read_number_into(ds8* buffer, Parser* p)
{
	for (p->ptr++; _isdigit(*p->ptr); p->ptr++)
		ds8_appendc(buffer, *p->ptr);
	p->ptr--;
}


static void
read_next_entry_into(ds8* buf, Parser* p, i32 ul)
{
	/* Skip to start */
	for (p->ptr++; *p->ptr == ':' || _isspace(*p->ptr); p->ptr++)
		;

	if (_isdigit(*p->ptr) || *p->ptr == '-') // Interpret - as negative sign
	{
		ds8_appendc(buf, *p->ptr);
		read_number_into(buf, p);
	}
	else if (*p->ptr == '"')
		read_string_into(buf, p);
	else if (*p->ptr == '[')
	{       /* read all entries contained in [] into str */
		bool stop = false;
		while (!stop)
		{
			read_next_entry_into(buf, p, ul);

			// Check if this was the last entry
			while (*(++p->ptr))
			{
				if (*p->ptr == ',')
				{       // Next incoming
					break;
				}
				else if (*p->ptr == ']')
				{
					stop = true;
					break;
				}
				assert(_isspace(*p->ptr));
			}
		}

		assert(*p->ptr == ']');
	}
	else if (*p->ptr == '{')
	{
		ds8 json_entry = ds8_new(50); // I think using the stack would be cleaner, but requires rewrite of read_string_into
		ds8 tag = ds8_new(50);
		int cbracket_lvl = 1;

		for (p->ptr++; *p->ptr; p->ptr++)
		{
			if (*p->ptr == '"' && cbracket_lvl == 1)
			{
				read_string_into(&json_entry, p);

				if (ds8_equals(json_entry, ds8("tag"))) // Expects "tag" to appear before "content"
				{
					read_next_entry_into(&tag, p, ul);

					if (ds8_equals(tag, ds8("ul")))
						ul++;
					else if (ds8_equals(tag, ds8("li")))
					{
						if (buf->len == 0 || buf->s[buf->len - 1] != '\n')
							ds8_appendc(buf, '\n');
						for (i32 i = 0; i < ul - 1; i++)
							ds8_appendc(buf, '\t');
						ds8_appendstr(buf, ds8("• "));
					}
					else if (ds8_equals(tag, ds8("div")))
					{
						if (buf->len == 0 || buf->s[buf->len - 1] != '\n')
							ds8_appendc(buf, '\n');
					}

					tag.len = 0;
				}
				else if (ds8_equals(json_entry, ds8("content")))
					read_next_entry_into(buf, p, ul);

				json_entry.len = 0;
			}
			else if (*p->ptr == '{')
				cbracket_lvl++;
			else if (*p->ptr == '}')
			{
				if (--cbracket_lvl <= 0)
				      break;
			}
		}
		ds8free(&json_entry);
		ds8free(&tag);

		assert(*p->ptr == '}');
	}
	else if (*p->ptr == ',' || *p->ptr == ']' || *p->ptr == '}')
		p->ptr--;
	else if (*p->ptr == 'n')
	{
		assert(u8compare((u8*)p->ptr, (u8*)"null", lengthof("null")) == 0);
		p->ptr += lengthof("ull");

		assert(*p->ptr == 'l');
	}
	else
		assert(false);
}

/*
 * Advances the pointer to the next ocurrence of @skip_c
 */
static void
skip_to(char skip_c, Parser* p)
{
	for (p->ptr++; *p->ptr && *p->ptr != skip_c; p->ptr++)
		;
}

static void
read_from_str(s8 dictname, Parser* p)
{
	if (*p->ptr != '[')
		skip_to('[', p);
	while (true)
	{
		skip_to('[', p);
		if (!*p->ptr)
			break;

		for (int i = 0; i < countof(entry_buffer); i++)
		{
			read_next_entry_into(&entry_buffer[i], p, 0);
			if (i != countof(entry_buffer) - 1)
				skip_to(',', p);
		}

		add_dictentry((s8dictentry) {
			.dictname = dictname,
			.kanji = (s8) { .s = entry_buffer[0].s, .len = entry_buffer[0].len },
			.reading = (s8) { .s = entry_buffer[1].s, .len = entry_buffer[1].len },
			.definition = (s8) { .s = entry_buffer[5].s, .len = entry_buffer[5].len }
		});

		/* dump_ds8v(countof(entry_buffer), entry_buffer); */

		ds8_reset_entries(entry_buffer);

		skip_to(']', p);
	}
}

s8
extract_dictname(zip_t* archive)
{
	struct zip_stat finfo;
	zip_stat_init(&finfo);

	zip_stat(archive, "index.json", 0, &finfo);
	s8 index_txt = { .s = alloca(finfo.size + 1), .len = finfo.size };

	struct zip_file* index_file = zip_fopen(archive, "index.json", 0);
	zip_fread(index_file, index_txt.s, index_txt.len);
	zip_fclose(index_file);

	// TODO: Write this properly
	char* search_string = "\"title\"";
	char* title_start = strstr((char*)index_txt.s, search_string);
	if (!title_start)
		return (s8){ 0 }
	;
	title_start += strlen(search_string);
	title_start = strstr(title_start, "\"") + 1; // No error handling

	char* title_end = strstr(title_start, "\"");

	assert(title_end - title_start >= 0);
	s8 r = { 0 };
	r.len = title_end - title_start,
	r.s = (u8*)g_strndup(title_start, r.len);
	return r;
}

static int
read_from_zip(char* filename)
{
	if (!fopen(filename, "r"))
		return -2;

	int errorp = 0;
	zip_t* archive = zip_open(filename, 0, &errorp);

	s8 dictname = extract_dictname(archive);

	printf("Processing dictionary: %s\n", dictname.s);

	struct zip_stat finfo;
	zip_stat_init(&finfo);
	for (int i = 0; (zip_stat_index(archive, i, 0, &finfo)) == 0; i++)
	{
		if (strncmp(finfo.name, "term_bank_", lengthof("term_bank_")) == 0)
		{
			s8 txt = (s8) { .len = finfo.size + 1 };
			txt.s = _malloc(finfo.size + 1);

			zip_file_t* f = zip_fopen_index(archive, i, 0);
			zip_fread(f, txt.s, finfo.size);

			/* ds8_unescape(&txt); */

			Parser p = { .ptr = txt.s };
			read_from_str(dictname, &p);

			free(txt.s);
		}
	}
	return 0;
}


int
main(int argc, char * argv[])
{
	char* db_path = NULL;

	if (argc > 1)
		db_path = argv[1];
	else
	{
		char const* data_dir = g_get_user_data_dir();
		db_path = g_build_filename(data_dir, "dictpopup", NULL);
		printf("Using default path: %s\n", db_path);
	}

	//TODO: Ask for confirmation first
	char* data_fp = g_build_filename(db_path, "data.mdb", NULL);
	remove(data_fp);
	free(data_fp);

	opendb(db_path);

	DIR *dir = opendir(".");
	if (dir == NULL)
	{
		perror("Error opening current directory");
		exit(1);
	}

	for (int i = 0; i < countof(entry_buffer); i++)
	{
		entry_buffer[i] = (ds8){ .s = _malloc(INITIAL_SIZE), .cap = INITIAL_SIZE };
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		size_t len = strlen(entry->d_name);
		if (len > 4 && strcmp(entry->d_name + len - 4, ".zip") == 0)
			read_from_zip(entry->d_name);
	}
	closedir(dir);
	closedb();

	char* lock_file = g_build_filename(db_path, "lock.mdb", NULL);
	remove(lock_file);
	//FIXME
	remove("/tmp/data.mdb");
	remove("/tmp/lock.mdb");
}
