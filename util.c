#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libnotify/notify.h>
#include <unistd.h>
#include <glib.h>

#include "util.h"

void
notify(char const *fmt, ...)
{
	char *msg;
	va_list argp;
	va_start(argp, fmt);
	vasprintf(&msg, fmt, argp);
	va_end(argp);

	NotifyNotification *n;
	notify_init("Basics");

	n = notify_notification_new(msg, NULL, NULL);
	notify_notification_set_app_name(n, "dictpopup");  /* Maybe set it to argv[0] */

	if (!notify_notification_show(n, NULL))
		fprintf(stderr, "ERROR: Failed to send notification.\n");

	g_object_unref(G_OBJECT(n));
}

/*
 * Returns an array of kanji readings
 * Supported formats:
 *
 *  * かむ【嚙む・嚼む・咬む】
 *
 * The returned string should be freed with g_strfreev()
 */
char **
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

/*
 * Simply returns the kanji part of the string
 *
 * The kanji part is expected to be enclosed in 【】
 *
 * The returned string should be freed.
 */
char *
extract_kanji(const char *str)
{
	char *start = strstr(str, "【");
	start = start ? start + strlen("【") : NULL;
	char *end = start ? strstr(start, "】") : NULL;

	return (start && end) ? g_strndup(start, end-start) : g_strdup(str);
}

char *
extract_reading(const char *str)
{
	char *end_read = strstr(str, "【");
	if (!end_read)
		return strdup(str);

	return strndup(str, end_read - str);
}

/*--------- String functions -------- */
void
str_repl_by_char(char *str, char *target, char repl_c)
{
	if (!str || !target)
	{
		fprintf(stderr, "ERROR: str_repl_by_char has been called with NULL values.");
		return;
	}

	size_t target_len = strlen(target);
	char *s = str, *e = str;

	do{
		if (!strncmp(e, target, target_len))
		{
			if (repl_c)
				*s++ = repl_c;
			e += target_len;
		}
	} while ((*s++ = *e++));
}

void
nuke_whitespace(char *str)
{
	str_repl_by_char(str, "\n", '\0');
	str_repl_by_char(str, "\t", '\0');
	str_repl_by_char(str, " ", '\0');
	str_repl_by_char(str, "　", '\0');
}

/*
 * Calls a command in printf style.
 *
 * Returns: Newly allocated string with the standard output of the command or NULL on error.
 */
char*
read_cmd_sync(char const *fmt, ...)
{
    char *cmd;
    va_list argp;
    va_start(argp, fmt);
    vasprintf(&cmd, fmt, argp);
    va_end(argp);
    
    char *standard_out = NULL;
    // Might want to switch to g_spawn_sync for safety
    g_spawn_command_line_sync(cmd, &standard_out, NULL, NULL, NULL);

    free(cmd);
    return standard_out;
}

/*
 * Calls a command in printf style fashion. 
 *
 * Returns: TRUE on success and FALSE if there was an error
 */
int
spawn_cmd_async(char const *fmt, ...)
{
    char *cmd;
    va_list argp;
    va_start(argp, fmt);
    vasprintf(&cmd, fmt, argp);
    va_end(argp);

    int retval = g_spawn_command_line_async(cmd, NULL);
    free(cmd);
    return retval;
}
