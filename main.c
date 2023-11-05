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

void
parse_json(char *json, struct dictentry *des, size_t *numde)
{
  char keywords[3][13] = {"\"dict\"", "\"word\"", "\"definition\""};

  size_t curde = 0;
  for (int i = 1; json[i] != '\0' && curde < MAX_NUM_OF_DICT_ENTRIES; i++)
  {
	for (int k=0; k < 3; k++)
	{
	    if (strncmp(json+i, keywords[k], strlen(keywords[k])) == 0)
	    {
		i += strlen(keywords[k]);
		while (json[i] != '"' || json[i-1] == '\\') i++; /* could leave string */
		i++;

		if (k == 0)
		  des[curde].dictname = json + i;
		else if (k == 1)
		  des[curde].word = json + i;
		else if (k == 2)
		{
		  if (json[i] == '\n') i += 1;
		  des[curde].definition = json + i;
		}

		while (json[i] != '"' || json[i-1] == '\\') i++;

		json[i++] = '\0';

		if (k == 2) curde++;
	    }
	}
  }
  *numde = curde;

  /* for (int i=0; i < *numde; i++) */
  /*   printf("dict: %s\nword: %s\ndefinition: %s\n", des[i].dictname, des[i].word, des[i].definition); */
}

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

/* 
   Return string needs to be freed afterwards 
*/
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

/* 
   Return string needs to be freed afterwards 
*/
char *
lookup(char *word)
{
    const char sdcv_str[] = "sdcv -nej --utf8-output ";
    char sdcv_cmd[strlen(sdcv_str) + MAX_WORD_LEN + 1];
    sprintf(sdcv_cmd, "%s%s", sdcv_str, word);
    return execscript(sdcv_cmd);
}

void
nuke_whitespace(char *str) /* Could be optimized */
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

char *
find_deinflection(char *word, int curdepth) /* TODO: Check for memory leak */
{
      if (curdepth > MAX_DEINFLECTION_DEPTH)
	  return NULL;

      char deinfw[MAX_WORD_LEN]; // FIXME: dynamic allocation, strdup(word) could be an option
      wchar_t **deinflections = deinflect(word);
      char *sdcv_json = NULL;

      for (int i = 0; i < MAX_DEINFLECTIONS && deinflections[i] && is_empty_json(sdcv_json); i++)
      {
	  wcstombs(deinfw, deinflections[i], MAX_WORD_LEN);
	  printf("Testing deinflection: %s\n", deinfw);
	  sdcv_json = lookup(deinfw);
      }

      if (is_empty_json(sdcv_json))
      {
	    for (int i = 0; i < MAX_DEINFLECTIONS && deinflections[i] && is_empty_json(sdcv_json); i++)
	    {
		wcstombs(deinfw, deinflections[i], MAX_WORD_LEN);
		sdcv_json = find_deinflection(deinfw, curdepth+1);
	    }
      }

	    
      free_deinfs(deinflections);
      return sdcv_json;
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

    char *sdcv_json = lookup(luw);

    if (is_empty_json(sdcv_json))
	sdcv_json = find_deinflection(luw, 1);

    if (is_empty_json(sdcv_json))
      die("No dictionary entry found.");

    str_repl(sdcv_json, "\\n", '\n');
    struct dictentry des[MAX_NUM_OF_DICT_ENTRIES];
    size_t numde;

    parse_json(sdcv_json, des, &numde); /* des reuses sdcv_json string. 危ない */

    size_t curde = 0;
    int ev = popup(des, numde, &curde);
#ifdef ANKI_SUPPORT
    if (ev == 1)
	addNote(luw, des[curde].word, des[curde].definition);
#endif

    return 0;
}
