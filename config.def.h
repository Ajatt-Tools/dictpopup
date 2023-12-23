#ifdef IMPORT_VARIABLES

#define NUMBER_POSS_ENTRIES 9
enum PossibleEntries {
	Empty,
	LookedUpString,
	CopiedSentence,
	BoldSentence,
	DictionaryKanji,
	DictionaryReading,
	DictionaryFurigana,
	DictionaryDefinition,
	FocusedWindowName
};

char *fieldnames[] = { "SentKanji", "VocabKanji", "VocabFurigana", "VocabDef", "Notes" };
int fieldmapping[] = { BoldSentence, DictionaryKanji, DictionaryFurigana, DictionaryDefinition, FocusedWindowName };
#endif // IMPORT_VARIABLES

/* Anki settings */
#define ANKI_DECK "Japanese::4 - Sentences"
#define ANKI_MODEL "Japanese sentences (Recognition)"
#define SEARCH_FIELD "VocabKanji"

/* Popup settings */
#define WIN_MARGIN 5
#define WIN_WIDTH 480
#define WIN_HEIGHT 300
