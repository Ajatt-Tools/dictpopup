#ifndef DEINFLECTOR_H
#define DEINFLECTOR_H

#define MAX_DEINFLECTIONS 5

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <stdbool.h>

#include "config.h"
#include "util.h"

void
free_deinfs(wchar_t **deinflections)
{
    for (int i = 0; i < MAX_DEINFLECTIONS; i++)
	free(deinflections[i]);
    /* free (deinflections); */
}

int
getFreeEntry(wchar_t **deinflections)
{
      int i = 0;
      while (i < MAX_DEINFLECTIONS && deinflections[i] != NULL)
	i++;
      if (i >= MAX_DEINFLECTIONS)
	  fprintf(stderr, "No free entry for deinflection found. Likely due to MAX_DEINFLECTIONS being too low.");
      return i;
}

void
add_replace(wchar_t **deinflections, wchar_t *word, wchar_t repl, size_t pos)
{
      int i = getFreeEntry(deinflections);
      deinflections[i] = wcsdup(word);
      deinflections[i][pos] = repl;
}

void
addatpos(wchar_t **deinflections, wchar_t *word, wchar_t chr, size_t pos)
{
      int i = getFreeEntry(deinflections);
      
      if(!(deinflections[i] = (wchar_t *)malloc((pos + 2) * sizeof(wchar_t))))
	  die("Could not assign memory for deinflection.");

      wmemcpy(deinflections[i], word, pos); /* Error if pos-1 > strlen(word)? */
      deinflections[i][pos] = chr;
      deinflections[i][pos+1] = L'\0';
}

wchar_t **
itou_atou_form(wchar_t **deinflections, wchar_t word[], int len, bool aform)
{
    word[len] = L'\0';
    wchar_t aforms[] = L"さかがばたまわなら";
    wchar_t iforms[] = L"しきぎびちみいにり";
    wchar_t uforms[] = L"すくぐぶつむうぬる";
    size_t num = wcslen(aforms); // expects same length

    wchar_t lastchr = word[len - 1];
    wchar_t *forms = aform? aforms : iforms;

    int i = 0;
    while (forms[i] != lastchr && i < num)
      i++;

    if (i < num)
	add_replace(deinflections, word, uforms[i], len - 1);
    if (!aform || i >= num) // For iform orig can be る-form or not, e.g. 生きます
	addatpos(deinflections, word, L'る', len);

    return deinflections;
}

/* Expects input to still contain the て */
wchar_t **
te_form(wchar_t **deinflections, wchar_t *word, int len)
{
    word[--len] = L'\0';
    wchar_t lastchr = word[len-1];

    switch (lastchr)
    {
	case L'し':
	    add_replace(deinflections, word, L'す', len - 1);
	    break;
	case L'い':
	    add_replace(deinflections, word, L'く', len - 1);
	    break;
	case L'ん':
	    add_replace(deinflections, word, L'す', len - 1);
	    add_replace(deinflections, word, L'ぶ', len - 1);
	    add_replace(deinflections, word, L'ぬ', len - 1);
	    break;
	case L'く': /* e.g. 行かなくて, not neseccary actually */
	    add_replace(deinflections, word, L'い', len - 1);
	case L'っ':
	      if (len >= 2 && word[len-2] == L'行')
		  add_replace(deinflections, word, L'く', len - 1);
	      else
	      {
		  add_replace(deinflections, word, L'る', len - 1);
		  add_replace(deinflections, word, L'う', len - 1);
		  add_replace(deinflections, word, L'つ', len - 1);
	      }
	    break;
	default:
	    addatpos(deinflections, word, L'る', len);
    }

    return deinflections;
}

/* Expects input to still contain the で */
wchar_t **
de_form(wchar_t **deinflections, wchar_t *word, int len)
{
    word[--len] = L'\0';
    wchar_t lastchr = word[len-1];

    switch (lastchr)
    {
	case L'い':
	    add_replace(deinflections, word, L'ぐ', len - 1);
	    break;
	case L'ん':
	    add_replace(deinflections, word, L'む', len - 1);
	    add_replace(deinflections, word, L'ぶ', len - 1);
	    add_replace(deinflections, word, L'ぬ', len - 1);
	    break;
	default:
	    notify("〜で suggests て-form, but could not find a match");
    }

    return deinflections;
}

void
deinflect_wc(wchar_t **deinflections, wchar_t *word)
{
  /* wchar_t **deinflections; */
  /* if(!(deinflections = (wchar_t **)calloc(MAX_DEINFLECTIONS, sizeof(wchar_t *)))) */
  /*     die("Could not allocate array for deinflections."); */

  int len = wcslen(word);
  if (len < 2)
    return;

  wchar_t *last3 = len >= 3 ? word + len - 3 : NULL;
  wchar_t *last2 = word + len - 2;
  wchar_t lastchr = word[len-1];

  if (last3 && !wcscmp(last3, L"しまう"))
  {
    add_replace(deinflections, word, L'\0', len - 3);
    return;
  }

  /* Different writings */
  if (word[0] == L'ご' || word[0] == L'お')
    add_replace(deinflections, word, L'御', 0);
  if (!wcscmp(last2, L"なし") || !wcscmp(last2, L"ない"))
    add_replace(deinflections, word, L'無', len-2);
  if (last3 && (!wcscmp(last3, L"かかり") || !wcscmp(last3, L"がかり")
	|| !wcscmp(last3, L"かかる") || !wcscmp(last3, L"かかる")))
    add_replace(deinflections, word, L'掛', len-3);
  /* --- */

  /* 動詞 - polite form */
  if(!wcscmp(last2, L"ます"))
  {
      itou_atou_form(deinflections, word, len - 2, 0);
      return;
  }
  else if(last3 && (!wcscmp(last3, L"ません") || !wcscmp(last3, L"ました")))
  {
      itou_atou_form(deinflections, word, len - 3, 0);
      return;
  }

  /* 形容詞 - 過去形 */
  if (last3 && !wcscmp(last3, L"かった"))
      addatpos(deinflections, word, L'い', len-3);
      /* No return, since can still be a verb, e.g. 授かった */

  /* 動詞 - volitional? */
  if (!wcscmp(word + len - 2, L"たい"))
  {
      itou_atou_form(deinflections, word, len - 2, 0);
      return;
  }

  /* 動詞 - passive */
  if (last3 && !wcscmp(last3, L"られる"))
  {
      word[len - 2] = L'\0';
      add_replace(deinflections, word, L'る', len - 3);
      return;
  }
  else if (!wcscmp(last2, L"れる"))
  {
      itou_atou_form(deinflections, word, len - 2, 1);
      return;
  }

  /* causative? */
  if (!wcscmp(last2, L"せる"))
  {
      itou_atou_form(deinflections, word, len - 2, 1);
      return;
  }

  /* 否定形 */
  if (!wcscmp(last2, L"ない"))
  {
      itou_atou_form(deinflections, word, len - 2, 1);
      return;
  }

  /* te form */
  switch (lastchr)
  {
      case L'て':
	te_form(deinflections, word, len);
	return;
      case L'で':
	de_form(deinflections, word, len);
	return;
  }

  /* 過去形 */
  switch (lastchr)
  {
      case L'た':
	te_form(deinflections, word, len);
	return;
      case L'だ':
	de_form(deinflections, word, len);
	return;
  }

  /* 形容詞 - ?? */
  switch (lastchr)
  {
      case L'く':

      case L'さ':
	add_replace(deinflections, word, L'い', len - 1);
	return;
  }
}

void
deinflect(wchar_t **deinflections, char *wordSTR)
{
  setlocale(LC_ALL, "");

  wchar_t word[MAX_WORD_LEN];
  mbstowcs(word, wordSTR, MAX_WORD_LEN);
  deinflect_wc(deinflections, word);
}

#endif // DEINFLECTOR_H
