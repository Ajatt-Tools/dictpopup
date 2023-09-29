#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <wchar.h>
#include <stdarg.h> // For va_start

#include "xlib.h"
#include "popup.h"
#include "anki.h"
#include "util.h"
#include "jsmn.h"
#include "config.h"
#include "deinflector.h"

static int
jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

void
conv_newline(char *str)
{
  /* Converts symbols "\n" to an actual newline in the string str */
  int len = strlen(str);
  int s = 0, e = 0;
  while (e + 1 < len)
  {
    if (str[e] == '\\' && str[e+1] == 'n')
    {
      if (s != 0) 
	str[s] = '\n';
      else
	s--; // Remove \n at beginning completely
      e++;
    }
    else
    {
      str[s] = str[e];
    }
    
    s++;
    e++;
  }

  str[s++] = str[e];
  str[s] = '\0';
}

char *
execscript(char *cmd)
{
	FILE *fp;
	fp = popen(cmd, "r");
	if (fp == NULL)
		die("Failed calling command %s.", cmd);
	static char *buf = NULL;
	static size_t size = 0;
	if(!getline(&buf, &size, fp)) // Only read first line !
		die("Failed reading output of the command: %s", cmd);
	pclose(fp);

	return buf;
}

int
main(int argc, char**argv)
{
    char *luw = (argc > 1) ? argv[1] : sselp(); // XFree sselp?

    if (luw[0] == '\0')
      die("No selection."); 
    else if (strlen(luw) > MAX_WORD_LEN)
      die("Lookup string is too long.");

    char sdcv_cmd[28 + MAX_WORD_LEN + 1]; // 28 is length of "sdcv -n --utf8-output -e -j "
    sprintf(sdcv_cmd, "sdcv -nej --utf8-output %s", luw);
    char *sdcv_json = execscript(sdcv_cmd);

    if (strncmp(sdcv_json, "[]", 2) == 0)
    { /* Try to deinflect */
	char deinfw[MAX_WORD_LEN]; // FIXME: dynamic allocation
	wchar_t **deinflections = deinflect(luw);
	for (int i = 0; i < 5 && deinflections[i]; i++)
	{
	    wcstombs(deinfw, deinflections[i], MAX_WORD_LEN);
	    sprintf(sdcv_cmd, "sdcv -nej --utf8-output %s", deinfw);
	    sdcv_json = execscript(sdcv_cmd);
	    if (strncmp(sdcv_json, "[]", 2))
	      break;
	}

	// Free deinflections
	for (int i = 0; i < 5 && deinflections[i]; i++)
	  free(deinflections[i]);
	free(deinflections);
    }

    if (strncmp(sdcv_json, "[]", 2) == 0)
      die("No dictionary entry found.");

    /* -- START JSON -- */
    char *de[MAX_NUM_OF_DICT_ENTRIES];
    char *word_de[MAX_NUM_OF_DICT_ENTRIES];
    int num_de = 0;

    int i, r;
    jsmn_parser p;
    jsmntok_t t[8 * MAX_NUM_OF_DICT_ENTRIES + 1]; // 1 entry ~ 7 json tokens + [ ... ]

    jsmn_init(&p);
    r = jsmn_parse(&p, sdcv_json, strlen(sdcv_json), t,
                 sizeof(t) / sizeof(t[0]));

    if (r == JSMN_ERROR_NOMEM)
      die("Too few tokens allocated. This is a bug.");
    else if (r == JSMN_ERROR_INVAL)
      die("Bad output from sdcv.");
    else if (r == JSMN_ERROR_PART)
      die("JSON string too short. Probably a bug in retrieving the command output.");

    // WARNING: Will break, if sdcv output changes
    for (i = 4; i < r; i++)
    {
	if (jsoneq(sdcv_json, &t[i], "word") == 0) {
	  word_de[num_de] = strndup(sdcv_json + t[i + 1].start, 
				    t[i + 1].end - t[i + 1].start);
	  i++;
	}
	else if (jsoneq(sdcv_json, &t[i], "definition") == 0) {
	  /* token_num[num_de] = i + 1; */
	  de[num_de++] = strndup(sdcv_json + t[i + 1].start, 
				 t[i + 1].end - t[i + 1].start);
	  i += 4;
	}
    }
    /* -- END JSON -- */

    for (int i = 0; i < num_de; i++)
      conv_newline(de[i]);
    
    int x, y;
    getPointerPosition(&x, &y);

    int ide = 0;
    int ev = popup(de, num_de, x, y);
#ifdef ANKI_SUPPORT
    if (ev == 1)
	addNote(luw, word_de[ide], de[ide]);
#endif

    free(sdcv_json);
    // TODO: free memory of strndup?
    return 0;
}
