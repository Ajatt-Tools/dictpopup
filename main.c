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
	  str[s] = str[e];
      
      s++;
      e++;
  }

  str[s++] = str[e];
  str[s] = '\0';
}

/* buf needs to be freed */
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

/* Return string needs to be freed afterwards */
char *
lookup(char *word)
{
    char sdcv_cmd[28 + MAX_WORD_LEN + 1]; // 28 is length of "sdcv -n --utf8-output -e -j "
    sprintf(sdcv_cmd, "sdcv -nej --utf8-output %s", word);
    return execscript(sdcv_cmd);
}

int
main(int argc, char**argv)
{
    char *luw = (argc > 1) ? argv[1] : sselp(); // XFree sselp?

    if (strlen(luw) == 0)
	die("No selection."); 
    else if (strlen(luw) > MAX_WORD_LEN)
	die("Lookup string is too long. Try increasing MAX_WORD_LEN.");

    char *sdcv_json = lookup(luw);

    if (strncmp(sdcv_json, "[]", 2) == 0)
    { 
	/* Try to deinflect */
	char deinfw[MAX_WORD_LEN]; // FIXME: dynamic allocation
	wchar_t **deinflections = deinflect(luw);
	for (int i = 0; i < MAX_DEINFLECTIONS && deinflections[i] && !strncmp(sdcv_json, "[]", 2); i++)
	{
	    //free(sdcv_json);
	    wcstombs(deinfw, deinflections[i], MAX_WORD_LEN); /* FIXME: ugly */
	    sdcv_json = lookup(deinfw);
	}

	// FIXME: Duplication
	if (strncmp(sdcv_json, "[]", 2) == 0)
	{
	    /* second round */
	    wchar_t **deinflections2;
	    for (int j = 0; j < MAX_DEINFLECTIONS && deinflections[j]; j++)
	    {
		deinflections2 = deinflect_wc(deinflections[j]);
		for (int i = 0; i < MAX_DEINFLECTIONS && deinflections2[i] && !strncmp(sdcv_json, "[]", 2); i++)
		{
		    wcstombs(deinfw, deinflections2[i], MAX_WORD_LEN);
		    sdcv_json = lookup(deinfw);
		}
	    }
	}
	/* -------- */
	
	wprintf(L"Testing the following deinflections:\n");
	for (int i = 0; i < MAX_DEINFLECTIONS && deinflections[i]; i++)
	    wprintf(L"%d: %ls\n", i + 1, deinflections[i]);

	// Free deinflections
	free_deinfs(deinflections);
	free(deinflections);
    }

    if (strncmp(sdcv_json, "[]", 2) == 0)
      die("No dictionary entry found.");

    /* -- START JSON -- */
    char *de[MAX_NUM_OF_DICT_ENTRIES];
    char *word_de[MAX_NUM_OF_DICT_ENTRIES];
    size_t num_de = 0;

    int i, r;
    jsmn_parser p;
    jsmntok_t t[7 * MAX_NUM_OF_DICT_ENTRIES + 5]; // 1 entry ~ 7 json tokens

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
    free(sdcv_json);

    for (int i = 0; i < num_de; i++)
      conv_newline(de[i]);
    
    int ev = popup(de, num_de);
#ifdef ANKI_SUPPORT
    if (ev == 1)
	addNote(luw, word_de[0], de[0]); // TODO: Add chosen de from popup
#endif

    // TODO: free dictionary entries?
    return 0;
}
