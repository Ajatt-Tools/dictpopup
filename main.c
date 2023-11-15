#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <wchar.h>
#include <stdarg.h> // For va_start

#include "xlib.h"
#include "anki.h"
#include "util.h"
#include "config.h"
#include "deinflector.h"
#include "structs.h"
#include "popup.h"
#include "kataconv.h"

void
str_repl(char *str, char *target, char repl)
{
  /* repl == '\0' means remove */
  int len = strlen(str);
  int len_t = strlen(target);
  int s = 0, e = 0;
  while (e + 1 < len)
  {
      if (strncmp(str + e, target, len_t) == 0)
      {
	  if (repl != '\0')
	    str[s] = repl;
	  else
	    s--;
	  e += len_t - 1;
      }
      else
	  str[s] = str[e];
      
      s++;
      e++;
  }

  str[s++] = str[e];
  str[s] = '\0';
}

char *
execscript(char *cmd)
{
	/* buf should be freed, but not if reused again like sdcv_output */
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

char *
lookup(char *word)
{
    const char sdcv_str[] = "sdcv -nej --utf8-output ";
    char sdcv_cmd[strlen(sdcv_str) + MAX_WORD_LEN + 1];
    sprintf(sdcv_cmd, "%s%s", sdcv_str, word);
    return execscript(sdcv_cmd);
}

void
nuke_whitespace(char *str) /* FIXME: Could be optimized, though marginal difference */
{
    str_repl(str, "\n", '\0');
    str_repl(str, " ", '\0');
    str_repl(str, "　", '\0');
}

int
is_empty_json(char *json)
{
    if (json == NULL || strlen(json) < 2)
      return 1;

    return (strncmp(json, "[]", 2) == 0);
}

void
add_dictionary_entry(struct dictentry **des, size_t *size_des, size_t *numde, struct dictentry de)
{
      if (*numde + 1 >= *size_des)
      {
	  *size_des += 5;
	  *des = reallocarray(*des, *size_des, sizeof(struct dictentry)); // FIXME: missing error handling
      }

      *numde += 1;
      (*des)[*numde - 1] = de;
}

void reset_dict(struct dictentry *dict)
{
    dict->dictname = NULL;
    dict->word = NULL;
    dict->definition = NULL;
}

void
add_from_json(struct dictentry **des, size_t *size_des, size_t *numde, char *json)
{
    /* Adds all dictionary entries from json to the dictentry array */

    str_repl(json, "\\n", '\n');
    char keywords[3][13] = {"\"dict\"", "\"word\"", "\"definition\""};

    struct dictentry curde = { NULL, NULL, NULL };
    int start, end;

    for (int i = 1; json[i] != '\0'; i++)
    {
	  for (int k=0; k < 3; k++)
	  {
		if (strncmp(json+i, keywords[k], strlen(keywords[k])) == 0)
		{
		    i += strlen(keywords[k]);
		    while (json[i] != '"' || json[i-1] == '\\') i++; /* could leave string */
		    i++;

		    while (json[i] == '\n') i++; // Skip leading newlines

		    start = i;

		    while (json[i] != '"' || json[i-1] == '\\') i++;

		    end = i;

		    if (k == 0)
			curde.dictname = strndup(json + start, end - start);
		    else if (k == 1)
			curde.word = strndup(json + start, end - start);
		    else if (k == 2)
			curde.definition = strndup(json + start, end - start);
		}
		if (curde.dictname && curde.word && curde.definition)
		{
		    add_dictionary_entry(des, size_des, numde, curde);
		    reset_dict(&curde);
		}
	  }
    }
}


void
add_deinflections(struct dictentry **des, size_t *size_des, size_t *numde, char *word, int curdepth)
{
      if (curdepth > MAX_DEINFLECTION_DEPTH)
	  return;

      char deinfw[MAX_WORD_LEN];
      wchar_t *deinflections[MAX_DEINFLECTIONS] = { NULL };
      deinflect(deinflections, word);
      char *sdcv_output = NULL;

      for (int i = 0; i < MAX_DEINFLECTIONS && deinflections[i]; i++)
      {
	  wcstombs(deinfw, deinflections[i], MAX_WORD_LEN);
	  printf("INFO: Looking up deinflection: %s\n", deinfw);
	  sdcv_output = lookup(deinfw);
	  if (!is_empty_json(sdcv_output))
	      add_from_json(des, size_des, numde, sdcv_output);

	  // Try to deinflect again, regardless of existing dict entry
	  add_deinflections(des, size_des, numde, deinfw, curdepth+1); 
      }
      free_deinfs(deinflections);
}

void
add_dictionaries(struct dictentry **des, size_t *numde, char *luw)
{
    size_t size_des = 5;
    if (!(*des = calloc(size_des, sizeof(struct dictentry))))
	die("Could not allocate memory for dictionary array.");

    char *sdcv_output = lookup(luw);
    add_from_json(des, &size_des, numde, sdcv_output);

    add_deinflections(des, &size_des, numde, luw, 1);
}

int
main(int argc, char**argv)
{
    char *luw = (argc > 1) ? argv[1] : sselp(); // XFree sselp?

    if (strlen(luw) == 0)
	die("No selection and no argument."); 
    else if (strlen(luw) > MAX_WORD_LEN)
	die("Lookup string is too long. Try increasing MAX_WORD_LEN.");

    nuke_whitespace(luw);

    struct dictentry *des;
    size_t numde = 0;
    add_dictionaries(&des, &numde, luw);

    if (numde == 0)
	die("No dictionary entry found.");

    size_t curde = 0;
    int rv = popup(des, numde, &curde);
#ifdef ANKI_SUPPORT
    if (rv == 1)
	addNote(luw, des[curde].word, des[curde].definition);
#endif

    return 0;
}
