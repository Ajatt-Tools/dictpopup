#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libnotify/notify.h>
#include <unistd.h>
#include <glib.h>
#include <gio/gio.h>

#include "util.h"

void
notify(bool urgent, char const *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	g_autofree char* msg = g_strdup_vprintf(fmt, argp);
	va_end(argp);

	// This is wack
	g_autoptr(GApplication) app = g_application_new("org.example.notification", G_APPLICATION_NON_UNIQUE);
	g_application_register(app, NULL, NULL);
	//

	GNotification *notification = g_notification_new("dictpopup");
	g_notification_set_body(notification, msg);
	if (urgent)
		g_notification_set_priority(notification, G_NOTIFICATION_PRIORITY_URGENT);
	g_application_send_notification(app, NULL, notification);

	/* NotifyNotification *n; */
	/* notify_init("Basics"); */

	/* n = notify_notification_new(msg, NULL, NULL); */
	/* notify_notification_set_app_name(n, "dictpopup");  /1* Maybe set it to argv[0] *1/ */
	/* if (error) */
	/* 	notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL); */

	/* if (!notify_notification_show(n, NULL)) */
	/* 	fprintf(stderr, "ERROR: Failed to send notification.\n"); */

	/* g_object_unref(G_OBJECT(n)); */
}


void
remove_last_unichar(char* str, int* len)
{
	// TODO: Implement this properly
	str[*len - 3] = '\0';
	*len -= 3;
}

char**
extract_kanji_array(const char *str)
{
	char *start = strstr(str, "【");
	start = start ? start + strlen("【") : NULL;
	char *end = start ? strstr(start, "】") : NULL;

	g_autoptr(GStrvBuilder) builder = g_strv_builder_new();

	if (!start || !end) // unknown format or no kanji.
		g_strv_builder_add(builder, str);
	else
	{
		char *delim;
		while ((delim = strstr(start, "・")))
		{
			g_strv_builder_add(builder, g_strndup(start, delim - start));
			start = delim + strlen("・");
		}
		g_strv_builder_add(builder, g_strndup(start, end - start));
	}

	return g_strv_builder_end(builder);
}

char *
extract_kanji(const char *str)
{
	char *start = strstr(str, "【");
	start = start ? start + strlen("【") : NULL;
	char *end = start ? strstr(start, "】") : NULL;

	return (start && end) ? g_strndup(start, end - start) : g_strdup(str);
}

char*
extract_reading(const char *str)
{
	char *end_read = strstr(str, "【");
	if (!end_read)
		return g_strdup(str);

	return g_strndup(str, end_read - str);
}

char*
str_repl_by_char(char *str, char *target, char repl_c)
{
	if (!str || !target)
	{
		g_warning("str_repl_by_char has been called with NULL values.");
		return str;
	}

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
