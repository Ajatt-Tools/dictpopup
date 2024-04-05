#include <limits.h> // INT_MAX

#include "dbreader.h"
#include "popup.h"
#include "util.h"
#include "messages.h"
#include "settings.h"
#include "deinflector.h"

static int
getfreq(s8 kanji, s8 reading)
{
      s8 key = concat(kanji, S("\0"), reading);
      int freq = db_getfreq(key);
      frees8(&key);
      if (freq == -1)
      {
	key = concat(kanji, S("\0"));
	freq = db_getfreq(key);
	frees8(&key);
      }
      if (freq == -1) freq = INT_MAX;

      return freq;
}

/*
 * Returns: dictentry with newly allocated strings parsed from @data
 */ 
static dictentry
data_to_dictent(s8 data)
{
    s8 d = data;

    s8 data_split[4] = { 0 };
    for (int i = 0; i < countof(data_split) && d.len > 0; i++)
    {
	size len = 0;
	for (; len < d.len; len++)
	{
	    if (d.s[len] == '\0')
		break;
	}
	data_split[i] = news8(len);
	u8copy(data_split[i].s, d.s, data_split[i].len);

	d.s += data_split[i].len + 1;
	d.len -= data_split[i].len + 1;
    }

    return (dictentry) {
	.dictname = data_split[0],
	.kanji = data_split[1],
	.reading = data_split[2],
	.definition = data_split[3],
	.frequency = getfreq(data_split[1], data_split[2])
    };
}

static void
lookup_add_to_dict(dictentry* dict[static 1], s8 word)
{
    debug_msg("Looking up: %.*s", (int)word.len, word.s);

    size_t n_ids = 0;
    u32* ids = db_getids(word, &n_ids);
    if (ids)
    {
	for (size_t i = 0; i < n_ids; i++)
	{
	    s8 de_data = db_getdata(ids[i]);
	    dictentry de = data_to_dictent(de_data);
	    dictionary_add(dict, de);
	}
    }
}

static void
add_deinflections_to_dict(dictentry* dict[static 1], s8 word)
{
    s8* deinfs_b = deinflect(word);
    for (size_t i = 0; i < buf_size(deinfs_b); i++)
	lookup_add_to_dict(dict, deinfs_b[i]);
    frees8buffer(&deinfs_b);
}

#include <string.h>
static int
indexof(char* str, char* arr[])
{
    if (str && arr)
    {
	for (int i = 0; arr[i]; i++)
	{
	    if (strcmp(str, arr[i]) == 0)
		return i;
	}
    }
    return INT_MAX;
}

static int
dictentry_comparer(void const* voida, void const* voidb)
{
    dictentry* a = (dictentry*)voida;
    dictentry* b = (dictentry*)voidb;
    assert(a && b);

    int inda = 0, indb = 0;
    if (s8equals(a->dictname, b->dictname))
    {
	inda = a->frequency;
	indb = b->frequency;
    }
    else
    {
	inda = indexof((char*)a->dictname.s, cfg.general.dictSortOrder);
	indb = indexof((char*)b->dictname.s, cfg.general.dictSortOrder);
    }

    return inda < indb ? -1
	   : inda == indb ? 0
	   : 1;
}

static void
fill_dictionary_with(dictentry* dict[static 1], s8 word)
{
    lookup_add_to_dict(dict, word);
    add_deinflections_to_dict(dict, word);
}

void
create_dictionary(s8 lookup[static 1])
{
    dictentry* dict = NULL;

    open_database();
    fill_dictionary_with(&dict, *lookup);
    if (dictlen(dict) == 0 && cfg.general.mecab)
    {
	s8 hira = kanji2hira(*lookup);
	fill_dictionary_with(&dict, hira);
	frees8(&hira);
    }
    if (dictlen(dict) == 0 && cfg.general.substringSearch)
    {
	while (dictlen(dict) == 0 && lookup->len > 3)
	{
	    *lookup = s8striputf8chr(*lookup);
	    fill_dictionary_with(&dict, *lookup);
	}
    }
    close_database();

    if (dictlen(dict) == 0)
    {
	msg("No dictionary entry found");
	exit(EXIT_FAILURE);
    }
    
    if (cfg.general.sort)
	qsort(dict, dictlen(dict), sizeof(dictentry), dictentry_comparer);

    lookup->s[lookup->len] = '\0';
    dictionary_data_done(dict);
}
