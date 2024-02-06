#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <glib.h>
#include <gio/gio.h>

#include "util.h"

#include <gtk/gtk.h>
char*
sselp()
{
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	return gtk_clipboard_wait_for_text(clipboard); // TODO: Maybe don't wait but quit if empty?
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

void
remove_last_unichar(size_t len[static 1], char str[*len])
{
	assert(*len > 0);

	--(*len);
	while (*len > 0 && (str[*len] & 0x80) != 0x00 && (str[*len] & 0xC0) != 0xC0)
		--(*len);
	str[*len] = '\0';
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

char*
read_cmd_sync(char** argv)
{
	char *standard_out = NULL;
	g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &standard_out, NULL, NULL, NULL);
	return standard_out;
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

int
printf_cmd_sync(char const *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	g_autofree char* cmd = g_strdup_vprintf(fmt, argp);
	va_end(argp);

	return system(cmd);
	/* return g_spawn_command_line_sync(cmd, NULL); */
}


// Old code from sdcv implementation. Might become usefull again if we support other dictionary formats
/* char** */
/* extract_kanji_array(const char *str) */
/* { */
/* 	char *start = strstr(str, "【"); */
/* 	start = start ? start + strlen("【") : NULL; */
/* 	char *end = start ? strstr(start, "】") : NULL; */

/* 	g_autoptr(GStrvBuilder) builder = g_strv_builder_new(); */

/* 	if (!start || !end) // unknown format or no kanji. */
/* 		g_strv_builder_add(builder, str); */
/* 	else */
/* 	{ */
/* 		char *delim; */
/* 		while ((delim = strstr(start, "・"))) */
/* 		{ */
/* 			g_strv_builder_add(builder, g_strndup(start, delim - start)); */
/* 			start = delim + strlen("・"); */
/* 		} */
/* 		g_strv_builder_add(builder, g_strndup(start, end - start)); */
/* 	} */

/* 	return g_strv_builder_end(builder); */
/* } */

/* char * */
/* extract_kanji(const char *str) */
/* { */
/* 	char *start = strstr(str, "【"); */
/* 	start = start ? start + strlen("【") : NULL; */
/* 	char *end = start ? strstr(start, "】") : NULL; */

/* 	return (start && end) ? g_strndup(start, end - start) : g_strdup(str); */
/* } */

/* char* */
/* extract_reading(const char *str) */
/* { */
/* 	char *end_read = strstr(str, "【"); */
/* 	if (!end_read) */
/* 		return g_strdup(str); */

/* 	return g_strndup(str, end_read - str); */
/* } */
