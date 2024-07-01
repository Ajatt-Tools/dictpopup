# dictpopup

This is a lightweight popup for searching selected text in your Yomichan dictionaries with support for creating Anki
cards. The look of the popup will depend on your gtk3 settings.

https://github.com/GenjiFujimoto/dictpopup/assets/50422430/353e5573-e728-419a-a2b8-058c8ed6da04

https://github.com/Ajatt-Tools/dictpopup/assets/50422430/61207a30-7838-4a30-a5b2-c4f6d6865821

## Current state

This is still very much a work in progress.
**It only works on the X window manager** right now (i.e. Linux with X11) and the parser for the Yomichan dictionaries
isn't fully implemented yet (for dictionaries with structured content).
See `TODO.txt` for more.

## List of features

* Deinflect
* Kanjify e.g. お前 -> 御前
* Yomichan-style lookup, i.e. decrease length of lookup until there is a match
* Sort dictionary entries by frequency (requires a frequency dictionary. I recommend CC100)
* Fall back to a hiragana conversion (for words written half in kanji / half in hiragana, e.g: かけ布団, 思いつく)
* Play a pronunciation on lookup / button press (requires files see [Pronunciation](#pronunciation))
* Add word with selected definition to Anki
* Indicator showing if a word already exists in your Anki deck / collection. Orange if existing cards are suspended.
  Anki browser search on press.
* Allow adding the window title to some Anki field (If you are adding from a book, this will e.g. most probably contain
  the book title)
* Fast and memory efficient

## Dependencies

lmdb, mecab, gtk3, libx11, curl, libnotify, libzip\
[AnkiConnect Anki addon](https://ankiweb.net/shared/info/2055492159) (for Anki support)\
ffplay (optional, for pronunciation)

## Installation

### Arch Linux

Install the AUR package `dictpopup`.

### Manual

First make sure, that you have all necessary dependencies installed. On a Debian based distro this can be done with:

```
sudo apt-get install liblmdb-dev libmecab-dev libgtk-3-dev libx11-dev \
     libcurl4-openssl-dev libnotify-dev libzip-dev zipcmp zipmerge ziptool
```

Then install with:

```bash
git clone "https://github.com/Ajatt-Tools/dictpopup.git"
cd dictpopup
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
sudo cmake --build build --target install -j 2
```

### Windows

The manual installation instructions above should also work for Windows under MSYS2, so someone
experienced might get it to run. But note that the default config location is currently hard coded for Linux.
I will provide an installer in the future.

## Setup

After the program installation you need to create a database where the dictionary entries are read from.
This is done via `dictpopup-create` which creates the database from all Yomichan dictionaries in the current directory.
Without arguments, it stores the database in the default location `~/.local/share/dictpopup` which is also the default
search path.

You can also include a frequency dictionary in there, but only one (it will give an error if you add more, because I
don't think that mixing makes sense).

## Configuration

Copy the example config `config.ini` of the repo to `~/.config/dictpopup/config.ini` and configure it according to your
setup.
The syntax follows the [Desktop Entry Specification](http://freedesktop.org/Standards/desktop-entry-spec).

Be careful to not include trailing spaces after your variables (for now).

## Pronunciation

To enable pronunciation, simply download some pronunciation indices from Ajatt-Tools (such
as [NHK 2016](https://github.com/Ajatt-Tools/nhk_2016_pronunciations_index))
and specify the path to the directory containing the downloaded folders in the config file. The expected folder
structure looks as follows:

```
 ├── ajt_japanese_audio
 │   ├── daijisen_pronunciations_index
 │   │   ├── index.json
 │   │   ├── media
 │   ├── nhk_1998_pronunciations_index
 │   │   ├── index.json
 │   │   ├── media
 │   │   └── release_info.conf
 │   ├── nhk_2016_pronunciations_index
 │   │   ├── index.json
```

On the first run it will automatically create an index which might take up some space (up to 200M if you have many
indices like I do).
This will allow for a faster lookup.

## Usage

Select a word and call `dictpopup` (using a shortcut). It is also possible to give an argument
instead: `dictpopup WORD`.
If something is not working as expected, you can add `DP_DEBUG=1` before the command or add the command line option `-c`
to print the config on stdout.

The "+" sign adds the currently shown definition to Anki after prompting you to copy a sentence.
If there is text selected in the popup window, it will be used instead as a definition.

The green/red dot indicates whether the word is already present in your Anki collection.
It displays an orange dot if there exist corresponding cards, but which are all suspended.

## Keybindings

- Next entry: <kbd>n</kbd>, <kbd>s</kbd>
- Previous entry: <kbd>p</kbd>, <kbd>a</kbd>
- Create an Anki card: <kbd>Ctrl</kbd>+ <kbd>s</kbd>
- Play audio: <kbd>r</kbd>
- Exit: <kbd>q</kbd>, <kbd>Esc</kbd>

## Contact

If you are having trouble setting up the program, don't hesitate to open up
an [issue](https://github.com/btrkeks/dictpopup/issues) or write me an [email](mailto:butterkeks@fedora.email).
