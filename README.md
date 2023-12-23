# dictpopup

This is a lightweight program for Linux showing a popup with the translation of the selected (japanese) text with support for creating Anki cards. 
It uses [sdcv](https://github.com/Dushistov/sdcv) for the dictionary lookup and gtk3 for displaying the popup.
Hence the look will depend on your gtk3 theme settings.

https://github.com/GenjiFujimoto/dictpopup/assets/50422430/4d22d4a2-e138-4bee-bc98-df93ce650e28

## Dependencies
sdcv, gtk3, glib, x11

## Setup
First setup [sdcv](https://github.com/Dushistov/sdcv) according to their github page. Copy `config.def.h` to `config.h` and change the configuration according to your setup.
Then install with `sudo make install`.  You can uninstall with `sudo make uninstall`.

## Usage
Select a word and call `dictpopup` (using a shortcut). It is also possible to give an argument instead: `dictpopup WORD`.

You can switch between dictionary entries by pressing the arrow keys or the shortcuts `n` (next) and `p` (previous).
The popup can be closed with `q` or `Esc`.

The "+" sign (or Ctrl+s) adds the currently shown definition to Anki after prompting you to select a sentence.
If there is a selection, it will be used instead as a definition.

The green/red dot indicates wether the word is already present in your Anki collection.

The sound symbol plays a pronunciation from a local database using [jppron](https://github.com/GenjiFujimoto/jppron).

## Problems / Limitations
* Currently there is only support for [targeted sentence cards](https://ankiweb.net/shared/info/1557722832).
* There is currently only support for GNU/Linux with x11. Extending support should be easy however, since the only platform dependent code is for retrieving the selection. PRs welcome.
* See TODO.txt file
