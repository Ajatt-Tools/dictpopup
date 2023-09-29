#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

#include "config.h"
#include "util.h"

#define MAX_DEINFLECTIONS 5

wchar_t **deinflect_wc(wchar_t *word);

void
second_deinf(wchar_t **deinflections)
{
    wchar_t **deinfs_new = deinflect_wc(deinflections[0]);
    if (deinfs_new[0] != NULL) // FIXME: Free old deinflections !!
      deinflections = deinfs_new;
}

int
getFreeEntry(wchar_t **deinflections)
{
      int i = 0;
      while (deinflections[i] != NULL)
	i++;
      return i;
}

void
add_replacelast(wchar_t **deinflections, wchar_t *word, wchar_t lastchr)
{
      int i = getFreeEntry(deinflections);
      int len = wcslen(word);
      deinflections[i] = wcsdup(word);
      deinflections[i][len-1] = lastchr;
}

void
add_addlast(wchar_t **deinflections, wchar_t *word, wchar_t lastchr)
{
      int i = getFreeEntry(deinflections);
      int len = wcslen(word);
      
      deinflections[i] = (wchar_t *)malloc((len + 2) * sizeof(wchar_t));
      wcscpy(deinflections[i], word);
      deinflections[i][len] = lastchr;
      deinflections[i][len+1] = L'\0';
}

wchar_t **
itou_atou_form(wchar_t **deinflections, wchar_t word[], int len, int aform)
{
    wchar_t aforms[] = L"さかがばたまわなら";
    wchar_t iforms[] = L"しきぎびちみいにり";
    wchar_t uforms[] = L"すくぐぶつむうぬる";
    int num = wcslen(aforms); // expects same length

    wchar_t lastchr = word[len - 1];
    wchar_t *forms = aform? aforms : iforms;

    int i = 0;
    while (forms[i] != lastchr && i < num)
      i++;

    if (i < num)
	add_replacelast(deinflections, word, uforms[i]);
    if (!aform || i >= num) // For iform orig can be る-form or not, e.g. 生きます
	add_addlast(deinflections, word, L'る');

    return deinflections;
}

void te_form(wchar_t **deinflections, wchar_t *word, int len)
{
    wchar_t lastchr = word[len-1];

    switch (lastchr)
    {
	case L'し':
	    add_replacelast(deinflections, word, L'す');
	    return;
	case L'い':
	    add_replacelast(deinflections, word, L'く');
	    return;
	case L'ん':
	    add_replacelast(deinflections, word, L'す');
	    add_replacelast(deinflections, word, L'ぶ');
	    add_replacelast(deinflections, word, L'ぬ');
	    return;
	case L'っ':
	    if (len >= 3 && word[len-3] == L'行')
		add_replacelast(deinflections, word, L'く');
	    else
	    {
		add_replacelast(deinflections, word, L'る');
		add_replacelast(deinflections, word, L'う');
		add_replacelast(deinflections, word, L'つ');
	    }
	    return;
	default:
	    add_addlast(deinflections, word, L'る');
	    second_deinf(deinflections);
    }
}

void
de_form(wchar_t **deinflections, wchar_t *word, int len)
{
    wchar_t lastchr = word[len-1];

    switch (lastchr)
    {
	case L'い':
	    add_replacelast(deinflections, word, L'ぐ');
	    return;
	case L'ん':
	    add_replacelast(deinflections, word, L'む');
	    add_replacelast(deinflections, word, L'ぶ');
	    add_replacelast(deinflections, word, L'ぬ');
	    return;
	default:
	    notify("〜で suggests て-form, but could not find a match");
    }

    return;
}

/* Returns 5 Pointers with possible deinflections. */
/* NULL means no deinflection found. */
wchar_t **
deinflect(char *wordSTR)
{
  setlocale(LC_ALL, "");

  wchar_t word[MAX_WORD_LEN];
  mbstowcs(word, wordSTR, MAX_WORD_LEN);
  return deinflect_wc(word);
}

wchar_t **
deinflect_wc(wchar_t *word)
{
  wchar_t **deinflections = (wchar_t **)malloc(MAX_DEINFLECTIONS * sizeof(wchar_t *));
  if (deinflections != NULL)
        for (int i = 0; i < MAX_DEINFLECTIONS; i++)
            deinflections[i] = NULL;

  int len = wcslen(word);
  if (len < 2)
    return deinflections;

  wchar_t *last3 = word + len - 3;
  wchar_t *last2 = word + len - 2;
  wchar_t lastchr = word[len-1];

  /* polite form */
  if(!wcscmp(last2, L"ます"))
  {
      word[len - 2] = L'\0';
      return itou_atou_form(deinflections, word, len - 2, 0);
  }
  if(!wcscmp(last3, L"ません") || !wcscmp(last3, L"ました"))
  {
      word[len - 3] = L'\0';
      return itou_atou_form(deinflections, word, len - 3, 0);
  }

  /* 否定形 */
  if (!wcscmp(last2, L"ない"))
  {
      word[len - 2] = L'\0';
      return itou_atou_form(deinflections, word, len - 2, 1);
  }

  /* te form */
  switch (lastchr)
  {
      case L'て':
	word[--len] = L'\0';
	te_form(deinflections, word, len);
	return deinflections;
      case L'で':
	word[--len] = L'\0';
	de_form(deinflections, word, len);
	return deinflections;
  }

  /* 過去形 */
  switch (lastchr)
  {
      case L'た':
	word[--len] = L'\0';
	te_form(deinflections, word, len);
	return deinflections;
      case L'だ':
	word[--len] = L'\0';
	de_form(deinflections, word, len);
	return deinflections;
  }

  return deinflections;
}
