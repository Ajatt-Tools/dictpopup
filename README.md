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
Copy the example config `config.ini` of the repo to `~/.config/dictpopup/config.ini` and configure it according to your setup. 
The syntax follows the [Desktop Entry Specification](http://freedesktop.org/Standards/desktop-entry-spec).

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
