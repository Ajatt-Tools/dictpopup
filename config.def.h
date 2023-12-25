#ifdef IMPORT_VARIABLES

#define NUMBER_POSS_ENTRIES 10
enum PossibleEntries {
	Empty,			/* An empty string. */
	LookedUpString,		/* The word this program was called with */
	DeinflectedLookup,	/* The deinflected version of the lookup string */
	CopiedSentence,		/* The copied sentence */
	BoldSentence,		/* The copied sentence with lookup string in bold */
	DictionaryKanji,	/* All kanji writings from dictionary entry */
	DictionaryReading,	/* The hiragana reading form the dictionary entry */
	DictionaryDefinition,	/* The chosen dictionary definition */
	DeinflectedFurigana,	/* The string: [DeinflectedLookup][DictionaryReading] */
	FocusedWindowName	/* The name of the focused window at lookup time */
};

char *fieldnames[] = { "SentKanji", "VocabKanji", "VocabFurigana", "VocabDef", "Notes" };
int fieldmapping[] = { BoldSentence, DeinflectedLookup, DeinflectedFurigana, DictionaryDefinition, FocusedWindowName };
#endif // IMPORT_VARIABLES

/* Anki settings */
#define ANKI_DECK "Japanese::Mining"
#define ANKI_MODEL "Japanese sentences"
#define SEARCH_FIELD "VocabKanji"

/* Popup settings */
#define WIN_MARGIN 5
#define WIN_WIDTH 480
#define WIN_HEIGHT 300
