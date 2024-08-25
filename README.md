# dictpopup

This is a lightweight popup for searching selected text in your Yomichan dictionaries with support for creating Anki
cards. The look of the popup will depend on your gtk3 settings.

https://github.com/GenjiFujimoto/dictpopup/assets/50422430/353e5573-e728-419a-a2b8-058c8ed6da04

https://github.com/Ajatt-Tools/dictpopup/assets/50422430/a0a631eb-85dd-4644-9001-10d2e1076ed4

## List of features

* Deinflect
* Yomichan-style lookup, i.e. decrease length of lookup until there is a match
* Sort dictionary entries by frequency (Requires a frequency dictionary. I recommend CC100)
* Fall back to a hiragana conversion (For words written half in kanji / half in hiragana, e.g: かけ布団, 思いつく)
* Play a pronunciations (requires files see [Pronunciation](#pronunciation))
* Add the word to Anki using either the visible definition, the mouse selection or clipboard contents
* Indicator showing if a word already exists in your Anki deck / collection.
* Allow adding the window title to some Anki field (If you are adding from a book, this will e.g. most probably contain
  the book title)
* Fast and memory efficient

## Dependencies

lmdb, mecab, gtk3, libx11, curl, libnotify, libzip, gperf\
[AnkiConnect Anki addon](https://ankiweb.net/shared/info/2055492159) (for Anki support)\
ffplay (optional, for pronunciation)

## Installation

### Arch Linux

Install the AUR package `dictpopup`.

### Manual

First make sure, that you have all necessary dependencies installed. On a Debian based distro this can be done with:

```
sudo apt-get install liblmdb-dev libmecab-dev libgtk-3-dev libx11-dev \
     libcurl4-openssl-dev libnotify-dev libzip-dev zipcmp zipmerge ziptool gperf
```

Then install with:

```bash
git clone "https://github.com/Ajatt-Tools/dictpopup.git"
cd dictpopup
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
sudo cmake --build build --target install -j 3
```

### Windows

The manual installation instructions above should also work for Windows under MSYS2, so someone
experienced might get it to run.

## Setup

After the program installation you need to first create an index where the dictionary entries are read from.
For this, open `dictpopup-config` (or `dictpopup` and then menu button > settings), then under 'Add dictionaries' in the
'General' tab, choose the folder that contains all of your Yomichan dictionaries and click 'Generate Index'.

You can also include a frequency dictionary in there, but only one (it will give an error if you add more, because I
don't think that mixing makes sense).

## Pronunciation

To enable pronunciation, simply download some pronunciation indices from Ajatt-Tools (such
as [NHK 2016](https://github.com/Ajatt-Tools/nhk_2016_pronunciations_index))
and generate an index under 'Add Pronunciation Files' in the 'Pronunciation' tab of the settings window. The expected
folder structure looks as follows:

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

By right clicking the pronunciation button, you can open a menu with all the available pronunciations
for the current kanji/reading pair.

## Usage

Select a word and call `dictpopup` (using a shortcut). It is also possible to give an argument
instead: `dictpopup WORD`.
If something is not working as expected, you can add `DP_DEBUG=1` before the command or add the command line option `-c`
to print the config on stdout.

The "+" sign adds the currently shown definition to Anki after prompting you to copy a sentence (if enabled).
If there is text selected in the popup window, it will be used instead as a definition.
Furthermore, by right-clicking the + sign, you can also use the current clipboard content as the definition instead.

## Keybindings

- Next entry: <kbd>n</kbd>, <kbd>s</kbd>
- Previous entry: <kbd>p</kbd>, <kbd>a</kbd>
- Create an Anki card: <kbd>Ctrl</kbd>+ <kbd>s</kbd>
- Play audio: <kbd>p</kbd>, <kbd>r</kbd>
- Exit: <kbd>q</kbd>, <kbd>Esc</kbd>

## Contact

If you are having trouble setting up the program, don't hesitate to open up
an [issue](https://github.com/btrkeks/dictpopup/issues) or write me an [email](mailto:butterkeks@fedora.email).
