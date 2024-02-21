#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <glib.h>
#include <gio/gio.h> // notifications

#include "util.h"

void* _malloc(size_t size)
{
	void* p = malloc(size);
	if (!p)
	{
		perror("malloc");
		abort();
	}
	return p;
}

void* _calloc(size_t nmemb, size_t size)
{
	void* p = calloc(nmemb, size);
	if (!p)
	{
		perror("calloc");
		abort();
	}
	return p;
}

void* _realloc(void* ptr, size_t size)
{
	void* p = realloc(ptr, size);
	if (!p)
	{
		perror("realloc");
		abort();
	}
	return p;
}


void
u8copy(u8 *dst, u8 *src, size n)
{
	assert(n >= 0);
	for (; n; n--)
	{
		*dst++ = *src++;
	}
}

i32
u8compare(u8 *a, u8 *b, size n)
{
	for (; n; n--)
	{
		i32 d = *a++ - *b++;
		if (d)
			return d;
	}
	return 0;
}

// Copy src into dst returning the remaining portion of dst.
s8
s8copy(s8 dst, s8 src)
{
	assert(dst.len >= src.len);

	u8copy(dst.s, src.s, src.len);
	dst.s += src.len;
	dst.len -= src.len;
	return dst;
}

s8
s8dup(s8 s)
{
	s8 r = (s8) {
		.s = _malloc(s.len),
		.len = s.len
	};
	u8copy(r.s, s.s, s.len);
	return r;
}

s8
s8fromcstr(char *z)
{
	s8 r = (s8) {
		.s = (u8*)z,
		.len = z ? strlen(z) : 0
	};

	assert(r.len >= 0); // overflow check
	return r;
}

i32
s8equals(s8 a, s8 b)
{
	return a.len == b.len && !u8compare(a.s, b.s, a.len);
}

void
s8striputf8chr(s8* s)
{
	// 0x80 = 10000000
	// 0xC0 = 11000000
	assert(s->len > 0);
	s->len--;
	while (s->len > 0 && (s->s[s->len] & 0x80) != 0x00 && (s->s[s->len] & 0xC0) != 0xC0)
		s->len--;
}

s8
s8unescape(s8 str)
{
	size s = 0, e = 0;
	while (e < str.len)
	{
		if (str.s[e] == '\\' && e + 1 < str.len)
		{
			switch (str.s[e + 1])
			{
			case 'n':
				str.s[s++] = '\n';
				e += 2;
				break;
			case '\\':
				str.s[s++] = '\\';
				e += 2;
				break;
			case '"':
				str.s[s++] = '"';
				e += 2;
				break;
			case 'r':
				str.s[s++] = '\r';
				e += 2;
				break;
			default:
				str.s[s++] = str.s[e++];
			}
		}
		else
			str.s[s++] = str.s[e++];
	}
	str.len = s;

	return str;
}


dictentry
dictentry_dup(dictentry de)
{
	return (dictentry){
		       .dictname = g_strdup(de.dictname),
		       .kanji = g_strdup(de.kanji),
		       .reading = g_strdup(de.reading),
		       .definition = g_strdup(de.definition)
	};
}

void
dictentry_free(void* ptr)
{
	dictentry* de = ptr;
	free(de->dictname);
	free(de->kanji);
	free(de->reading);
	free(de->definition);
	free(de);
}

void
dictentry_print(dictentry de)
{
	printf("dictname: %s\n"
	       "kanji: %s\n"
	       "reading: %s\n"
	       "definition: %s\n",
	       de.dictname,
	       de.kanji,
	       de.reading,
	       de.definition);
}


dictionary*
dictionary_new(void)
{
	return g_ptr_array_new_with_free_func(dictentry_free);
}

void
dictionary_copy_add(dictionary* dict, dictentry de)
{
	dictentry* de_dup = new(dictentry, 1);
	*de_dup = dictentry_dup(de);
	g_ptr_array_add(dict, de_dup);
}

void
dictionary_free(dictionary* dict)
{
	g_ptr_array_free(dict, TRUE);
}

dictentry
dictentry_at_index(dictionary *dict, unsigned int index)
{
	dictentry* de = g_ptr_array_index(dict, index);
	return *de;
}

DictComparer global_compare_func; // Looks very dagerous
gint g_compare_func(gconstpointer a, gconstpointer b)
{
	dictentry* entry1 = *((dictentry **)a);
	dictentry* entry2 = *((dictentry **)b);

	return global_compare_func(entry1, entry2);
}

void
dictionary_sort(dictionary* dict, DictComparer dict_compare_func)
{
	global_compare_func = dict_compare_func;
	g_ptr_array_sort(dict, g_compare_func);
}

void
dictionary_print(dictionary* dict)
{
	for (unsigned int i = 0; i < dict->len; i++)
		dictentry_print(dictentry_at_index(dict, i));
}

void
notify(bool urgent, char const *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	g_autofree char* msg = g_strdup_vprintf(fmt, argp);
	va_end(argp);

	GNotification *notification = g_notification_new("dictpopup");
	g_notification_set_body(notification, msg);
	if (urgent)
		g_notification_set_priority(notification, G_NOTIFICATION_PRIORITY_URGENT);

	// This is wack
	g_autoptr(GApplication) app = g_application_new("org.gtk.example", G_APPLICATION_NON_UNIQUE);
	g_application_register(app, NULL, NULL);
	//
	g_application_send_notification(app, NULL, notification);
}

// Returns false if there was an error
bool
check_ac_response(retval_s ac_resp)
{
	if (!ac_resp.ok)
	{
		assert(ac_resp.data.string);
		notify(1, "%s", ac_resp.data.string);
		fprintf(stderr, "%s", ac_resp.data.string);
		fprintf(stderr, "\n");
		return false;
	}

	return true;
}

char*
str_repl_by_char(char *str, char *target, char repl_c)
{
	assert(str && target);

	size_t target_len = strlen(target);
	char *s = str, *e = str;

	do{
		while (strncmp(e, target, target_len) == 0)
		{
			if (repl_c)
				*s++ = repl_c;
			e += target_len;
		}
	} while ((*s++ = *e++));

	return str;
}

void
nuke_whitespace(char *str)
{
	str_repl_by_char(str, "\n", '\0');
	str_repl_by_char(str, "\t", '\0');
	str_repl_by_char(str, " ", '\0');
	str_repl_by_char(str, "　", '\0');
}

int
printf_cmd_async(char const *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	g_autofree char* cmd = g_strdup_vprintf(fmt, argp);
	va_end(argp);

	return g_spawn_command_line_async(cmd, NULL);
}
