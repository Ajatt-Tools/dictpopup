#define MAX_WORD_LEN 50
/* 
Specifies max number of deinflection steps to be performed.
eg. 食べてしまった -1-> 食べてしまう -2-> 食べて -3-> 食べる
*/
#define MAX_DEINFLECTION_DEPTH 4

/* Anki settings */
#define ANKI_DECK "Japanese"
#define ANKI_MODEL "Japanese sentences"
#define ANKI_SENTENCE_FIELD "SentKanji"
#define ANKI_KANJI_FIELD "VocabKanji"
#define ANKI_FURIGANA_FIELD "VocabFurigana"
#define ANKI_DEFINITION_FIELD "VocabDef"
#define ANKI_NOTES_FIELD "Notes"
#define ANKI_API_URL "http://localhost:8765"

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
