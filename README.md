# dictpopup

This is a lightweight program for Linux showing a popup with the translation of the selected (japanese) text with support for creating Anki cards. 
It uses [sdcv](https://github.com/Dushistov/sdcv) for the dictionary lookup and gtk3 for displaying the popup.
Hence the look will depend on your gtk3 theme settings.
Furthermore it uses multithreading to achieve minimal startup time.

https://github.com/GenjiFujimoto/dictpopup/assets/50422430/4d22d4a2-e138-4bee-bc98-df93ce650e28

## Dependencies
sdcv, gtk3, glib, x11

## Setup
First setup [sdcv](https://github.com/Dushistov/sdcv) according to their github page. Copy `config.def.h` to `config.h` and change the configuration according to your setup.
Then install by running `./install.sh`. You can uninstall with `sudo make uninstall`.

## Configuration
Copy the example below into `~/.config/dictpopup/config` and configure it according to your setup. 
The syntax follows the [Desktop Entry Specification](http://freedesktop.org/Standards/desktop-entry-spec).
```
[Anki]
Deck = Japanese::4 - Sentences
NoteType = Japanese sentences (Recognition)
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

The sound symbol plays a pronunciation from a local database using [jppron](https://github.com/GenjiFujimoto/jppron).

## Problems / Limitations
* There is currently only support for GNU/Linux with x11. Extending support should be easy however, since the only platform dependent code is for retrieving the selection. PRs welcome.
* See TODO.txt file
