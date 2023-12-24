#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

#include <libnotify/notify.h>
#include <unistd.h>

void
notify(const char *message)
{
        NotifyNotification *n;

        notify_init ("Basics");

        n = notify_notification_new (message, NULL, NULL);
	notify_notification_set_app_name (n, "dictpopup"); /* Maybe set to argv[0] */
        /* notify_notification_set_timeout (n, 3000); */

        if (!notify_notification_show (n, NULL))
                fprintf (stderr, "failed to send notification\n");

	g_object_unref (G_OBJECT (n));
}

void
die(const char *fmt, ...) {
	char buffer[512];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);

	notify(buffer);
	fprintf(stderr, "%s\n", buffer);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		perror(NULL); // Only to stderr
	}

	exit(1);
}

char *
extract_kanji(const char *str)
{
    // TODO: Fix things like 嚙む・嚼む・咬む by looking at luw
    char *start_kanji, *end_kanji;

    start_kanji = strstr(str, "【");
    if (!start_kanji || !(end_kanji = strstr(start_kanji, "】")))
      return strdup(str);

    start_kanji += strlen("【");
    return strndup(start_kanji, end_kanji - start_kanji);
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
  if(!str || !target)
  {
    fprintf(stderr, "ERROR: str_repl_by_char has been called with NULL values.");
    return;
  }

  size_t target_len = strlen(target);
  char *s = str, *e = str;

  do 
  {
      if (!strncmp(e, target, target_len))
      {
	if (repl_c) 
	  *s++ = repl_c;
	e += target_len;
      }
  } while((*s++ = *e++));
}

void
nuke_whitespace(char *str)
{
    str_repl_by_char(str, "\n", '\0');
    str_repl_by_char(str, "\t", '\0');
    str_repl_by_char(str, " ", '\0');
    str_repl_by_char(str, "　", '\0');
}
