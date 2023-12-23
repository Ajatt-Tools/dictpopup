# dictpopup

This is a lightweight program showing a popup with the translation of the selected (japanese) text with support for creating anki cards. It uses [sdcv](https://github.com/Dushistov/sdcv) for the dictionary lookup and gtk3 for displaying the popup.
There is currently only support for GNU/Linux with x11, but extending support should be possible.

https://github.com/GenjiFujimoto/dictpopup/assets/50422430/4d22d4a2-e138-4bee-bc98-df93ce650e28

## Dependencies
sdcv, gtk3

## Setup
First setup [sdcv](https://github.com/Dushistov/sdcv) according to their github page. Copy `config.def.h` to `config.h` and change the configuration according to your setup.
Then install with `sudo make install`.  You can uninstall with `sudo make uninstall`.

If you would like to automatically lookup a word on every selection, then take a look at the following script: https://github.com/GenjiFujimoto/shell-scripts/blob/main/selautolookup

## Usage
Call `dictpopup word`.\
If no word is supplied, the selection will be used.\
You can switch between dictionary entries by pressing `n` (next) or `p` (previous) and
close the popup with `q` or `Esc`.

Anki cards can be created by pressing `Ctrl+s` and then selecting the sentence where
you encountered the word in.
Currently there is only support for [targeted sentence cards](https://ankiweb.net/shared/info/1557722832).
