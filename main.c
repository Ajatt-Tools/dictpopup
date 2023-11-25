#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <wchar.h>

#include "xlib.h"
/* #include "ankiconnect.h" */
#include "util.h"
#include "config.h"
#include "deinflector.h"
#include "structs.h"
#include "popup.h"
#include "ankiconnect.h"

void
str_repl(char *str, char *target, char repl)
{
  /* repl == '\0' means remove */
  if (!str || !target)
    die("str_repl function received NULL strings.");

  size_t len = strlen(str);
  size_t len_t = strlen(target);
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
	if(!getline(&buf, &size, fp)) // Only reads first line !
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
nuke_whitespace(char *str)
{
    /* FIXME: Could be optimized, though marginal difference */
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
add_dictionary_entry(dictentry **des, size_t *size_des, size_t *numde, dictentry de)
{
      if (*numde + 1 >= *size_des)
      {
	  *size_des += 5;
	  *des = reallocarray(*des, *size_des, sizeof(dictentry)); // FIXME: missing error handling
      }

      *numde += 1;
      (*des)[*numde - 1] = de;
}

void reset_dict(dictentry *dict)
{
    dict->dictname = NULL;
    dict->word = NULL;
    dict->definition = NULL;
}

void
add_from_json(dictentry **des, size_t *size_des, size_t *numde, char *json)
{
    /* Adds all dictionary entries from json to the des array */

    str_repl(json, "\\n", '\n');
    char keywords[3][13] = {"\"dict\"", "\"word\"", "\"definition\""};

    dictentry curde = { NULL, NULL, NULL };
    int start, end;
    char **entry;

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
			entry = &curde.dictname;
		    else if (k == 1)
			entry = &curde.word;
		    else
			entry = &curde.definition;

		    *entry = strndup(json + start, end - start);
		}
		if (curde.dictname && curde.word && curde.definition) // WARNING: Expects to see all fields before next dict entry
		{
		    add_dictionary_entry(des, size_des, numde, curde);
		    reset_dict(&curde);
		}
	  }
    }
}


void
add_deinflections(dictentry **des, size_t *size_des, size_t *numde, char *word, int curdepth)
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
add_dictionaries(dictentry **des, size_t *numde, char *luw)
{
    size_t size_des = 5;
    if (!(*des = calloc(size_des, sizeof(dictentry))))
	die("Could not allocate memory for dictionary array.");

    char *sdcv_output = lookup(luw);
    add_from_json(des, &size_des, numde, sdcv_output);

    add_deinflections(des, &size_des, numde, luw, 1);
}

void
edit_wname(char *wname)
{
    /* Strips unnecessary stuff from the windowname */
    // TODO
}

char *
boldWord(char *sent, char *word)
{
    char *bdword, *bdsent;
    if(asprintf(&bdword, "<b>%s</b>", word) == -1)
      die("Could not allocate memory for bold word."); // Not dying would be better
    bdsent = repl_str(sent, word, bdword);
    free(bdword);

    return bdsent;
}

char *
nuke(char *str)
{
    if (!NUKE_SPACES && !NUKE_NEWLINES)
      return str;

    int len = strlen(str);
    int skip = 0;
    for (int i = 0; i < len; i++)
    {
	if(NUKE_SPACES && (str[i] == ' ' || strcmp(str, "　") == 0))
	  skip++;
	else if (NUKE_NEWLINES && str[i] == '\n')
	  skip++;
	else
	  str[i-skip] = str[i];
    }
    str[len-skip] = '\0';

    return str;
}

static char *
extract_kanji(char *str, char *luw)
{
    // TODO: Fix things like 嚙む・嚼む・咬む by looking at luw
    char *start_kanji = strstr(str, "【");
    char *end_kanji = strstr(start_kanji, "】");
    if (!start_kanji || !end_kanji)
    {
      notify("Dictionary might contain unknown word formating (kanji).");
      return str;
    }
    else
      start_kanji += strlen("【");

    return strndup(start_kanji, end_kanji - start_kanji);
}

static char *
extract_reading(char *str)
{
    char *end_read = strstr(str, "【");

    if (end_read != NULL)
	return strndup(str, end_read - str);
    else /* Either different format or no kanji */
    {
      notify("Dictionary might contain unknown word formating (reading).");
      return str;
    }
}

void
populate_possible_entries(char *pe[], char *luw, dictentry de, char *def, char *winname)
{
    /* looked up string */
    pe[LookedUpString] = luw;

    /* sentence */
    notify("Please select the sentence.");
    clipnotify();
    pe[CopiedSentence] = nuke(sselp());

    /* bold sentence */
    pe[BoldSentence] = boldWord(pe[CopiedSentence], luw);

    /* dictionary word */
    pe[DictionaryKanji] = extract_kanji(de.word, luw);

    pe[DictionaryReading] = extract_reading(de.word);

    /* dictionary furigana */
    // TODO: Obviously won't work for 送り仮名 like 取り組む
    asprintf(&pe[DictionaryFurigana], "%s[%s]", pe[DictionaryKanji], pe[DictionaryReading]);

    /* dictionary entry */
    pe[DictionaryDefinition] = repl_str(def, "\n", "<br>");

    /* WindowName */
    if (winname == NULL)
      winname = "";
    pe[FocusedWindowName] = winname;
}

void
print_possible_entries(char *pe[])
{
  /* This is for debug purposes. */
  for (int i = 0; i < NUMBER_POSS_ENTRIES; i++)
  {
      switch(i)
      {
	case LookedUpString:
	  printf("LookedUpString");
	  break;
	case CopiedSentence:
	  printf("CopiedSentence");
	  break;
	case BoldSentence:
	  printf("BoldSentence");
	  break;
	case DictionaryKanji:
	  printf("DictionaryKanji");
	  break;
	case DictionaryReading:
	  printf("DictionaryReading");
	  break;
	case DictionaryFurigana:
	  printf("DictionaryFurigana");
	  break;
	case DictionaryDefinition:
	  printf("DictionaryDefinition");
	  break;
	case FocusedWindowName:
	  printf("FocusedWindowName");
	  break;
	default:
	  printf("Unknown Entry");
      }
      printf(": %s\n", pe[i]);
  }
}
  


int
main(int argc, char**argv)
{
    char *luw = (argc > 1) ? argv[1] : sselp(); // XFree sselp?
    char *wname = getwindowname();
    edit_wname(wname);

    if (strlen(luw) == 0)
	die("No selection and no argument."); 
    else if (strlen(luw) > MAX_WORD_LEN)
	die("Lookup string is too long. Try increasing MAX_WORD_LEN.");

    nuke_whitespace(luw);

    dictentry *des;
    size_t numde = 0;
    add_dictionaries(&des, &numde, luw);

    if (numde == 0)
	die("No dictionary entry found.");

    char *def;
    size_t de_num;

    int rv = popup(des, numde, &def, &de_num);

    if (rv == 1)
    {
	char *pe[NUMBER_POSS_ENTRIES];
	populate_possible_entries(pe, luw, des[de_num], def, wname);
	/* print_possible_entries(pe); */
	addNote(pe);
    }

    if (wname[0] != '\0')
	free(wname);
    return 0;
}
