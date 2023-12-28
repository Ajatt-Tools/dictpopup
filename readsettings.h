#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct {
	// Anki
	char *deck;
	char *notetype;
	char **fieldnames;
	size_t num_fields;
	int *fieldmapping;
	char *searchfield;
	// Popup
	unsigned int win_width;
	unsigned int win_height;
	unsigned int win_margin;
	// Behaviour
	gboolean ankisupport;
	gboolean checkexisting;
	gboolean copysentence;
	gboolean nukewhitespace;
	gboolean pronunciationbutton;
} settings;

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

void print_settings(settings *cfg);
settings* read_user_settings();
void free_settings(settings *cfg);

#endif /* SETTINGS_H */
