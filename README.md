# dictpopup

This is a lightweight popup for searching selected text in your yomichan dictionaries with support for creating Anki cards.
The look of the popup will depend on your gtk3 settings.

https://github.com/GenjiFujimoto/dictpopup/assets/50422430/4d22d4a2-e138-4bee-bc98-df93ce650e28

## Current state
This is still very much a work in progress.
**It only works on Linux** right now and the parser for the Yomichan dictionaries isn't fully implemented yet (for dictionaries with structured content).
See `TODO.txt` for more.

## List of features
 * Fast startup time
 * Very low memory footprint / CPU usage
 * Deinflect
 * Kanjify e.g. お前 -> 御前
 * Yomichan-style lookup, i.e. decrease length of lookup until there is a match
 * Try a hiragana conversion (for words written half in kanji / half in hiragana, e.g: かけ布団, 思いつく)
 * Play a pronunciation on lookup / button press (requires setup as for now)
 * Add word with selected definition to Anki
 * Indicator showing if a word already exists in your Anki deck / collection. Orange if the only existing cards are suspended
 * Allow adding the window title to some Anki field (If you are adding from a book, this will e.g. most probably contain the book title)

## Dependencies
gtk3, glib, x11

## Installation
First install by running `./install.sh`. (You can uninstall with `sudo make uninstall`)

Then you need to create the database where the entries are read from.
This is done via `dictpopup_create` which takes all Yomichan dictionaries in the current directory and puts them into the database.

## Configuration
Copy the example below into `~/.config/dictpopup/config.ini` and configure it according to your setup. 
The syntax follows the [Desktop Entry Specification](http://freedesktop.org/Standards/desktop-entry-spec).
```
[Anki]
Deck = Japanese
NoteType = Japanese sentences
# Available entries for the field mapping:
#
# 0	Empty			 An empty string.
# 1	LookedUpString		 The selected word or the argument dictpopup was called with
# 2	DeinflectedLookup	 The deinflected version of the lookup string
# 3	CopiedSentence		 The copied sentence
# 4	BoldSentence		 The copied sentence with the looked up string in bold
# 5	DictionaryKanji		 All kanji writings from the chosen dictionary entry, e.g. 嚙む・嚼む・咬む
# 6	DictionaryReading	 The hiragana reading form the dictionary entry
# 7	DictionaryDefinition	 The chosen dictionary definition
# 8	DeinflectedFurigana	 Currently the string: [DeinflectedLookup][DictionaryReading]
# 9	FocusedWindowName	 The name of the focused window at lookup time
#
FieldNames = SentKanji;VocabKanji;VocabFurigana;VocabDef;Notes;
FieldMapping = 4;2;8;7;9;
SearchField = VocabKanji

[Popup]
Width = 500
Height = 350
Margin = 5

[Database]
# You can set an alternative path. The default is ~/.local/share/dictpopup
# Path = 

[Behaviour]
AnkiSupport = true
CheckIfExists = true
CopySentence = true
NukeWhitespace = true
PronunciationButton = true
```
Be careful to not include trailing spaces after your variables (for now).

## Usage
Select a word and call `dictpopup` (using a shortcut). It is also possible to give an argument instead: `dictpopup WORD`.

You can switch between dictionary entries by clicking the arrow keys or by pressing the shortcuts `n` (next) and `p` (previous).
The popup can be closed with `q` or `Esc`.

The "+" sign (or Ctrl+s) adds the currently shown definition to Anki after prompting you to select a sentence.
If there is a selection, it will be used instead as a definition.

The green/red dot indicates wether the word is already present in your Anki collection.
It displays an orange dot if there exist cards, but which are all suspended.

The sound symbol plays a pronunciation from a local database using [jppron](https://github.com/GenjiFujimoto/jppron).
