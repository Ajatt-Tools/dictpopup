#ifndef CONFIG_H
#define CONFIG_H
// Don't change this
enum PossibleEntries { Empty, LookedUpString, CopiedSentence, BoldSentence, DictionaryKanji, DictionaryReading, DictionaryFurigana, DictionaryDefinition, FocusedWindowName };
#define NUMBER_POSS_ENTRIES 9

// Here you can configure your anki field names and the content which should be added to them
// Currently it expects exactly 5 fields!
static const char* fieldnames[] = { "SentKanji", "VocabKanji", "VocabFurigana", "VocabDef", "Notes" };
static const int fieldentry[] = { BoldSentence, DictionaryKanji, DictionaryFurigana, DictionaryDefinition, FocusedWindowName  };

#define MAX_WORD_LEN 50
/* 
Specifies max number of deinflection steps to be performed.
eg. 食べてしまった -1-> 食べてしまう -2-> 食べて -3-> 食べる
*/
#define MAX_DEINFLECTION_DEPTH 4

/* Anki settings */
#define ANKI_API_URL "http://localhost:8765"
#define ANKI_DECK "Japanese::4 - Sentences"
#define ANKI_MODEL "Japanese sentences"
static const char SEARCH_FIELD[] =  "VocabKanji"; // The word field which will be searched for duplicates

/* Popup settings */
#define FONT_SIZE "15pt" /* CSS syntax */
#define WIN_MARGIN 5
#define WIN_MARGIN 5
#define WIN_WIDTH 480
#define WIN_HEIGHT 300

/* Input settings */
#define NUKE_SPACES 1
#define NUKE_NEWLINES 1


/* features */
#define ANKi_SUPPORT
#define SDCV_DICTIONARY_CHECK

#endif // CONFIG_H
