# dictpopup

This is a very lightweight program to show a popup with the translation of the selected text. It uses [sdcv](https://github.com/Dushistov/sdcv) for the dictionary lookup and a slightly modified version of [herbe](https://github.com/dudik/herbe) to display the popup.

![image](https://github.com/GenjiFujimoto/dictpopup/assets/50422430/c4a3663b-fd91-4a66-95ad-f1528071c932)

![screenshot-2023-05-28-08-34-19](https://github.com/GenjiFujimoto/dictpopup/assets/50422430/179ad9f9-b4fa-4731-92c6-105ced37c353)

## Dependencies
sdcv, xclip (for selection support). \
Optional: lynx (for html support), clipnotify (for anki support). For Arch users: You will need `clipnotify-git` from the AUR instead `clipnotify` from the official repos.

## Setup
First setup [sdcv](https://github.com/Dushistov/sdcv) according to their github page. Optionally copy `config.def.h` to `config.h` and change the configuration.
Then install with `sudo make install`.  You can uninstall with `sudo make uninstall`.

f you would like to automatically lookup a word on every selection, then take a look at the following script: https://github.com/GenjiFujimoto/shell-scripts/blob/main/selautolookup

### Anki support
To enable anki support, open `dictpopup`, set `ANKK_SUPPORT` to `1` and edit the field names according to your anki setup. You can add a word by middle clicking the popup and then selecting the sentence you want to add.

## Usage
Call `dictpopup [html] [<WORD>]`.\
If no word as an argument is supplied, the selection is used.\
If the string `html` is supplied as the first argument, then html support will be enabled.\
The popup can be dismissed by clicking on it.

The styling can be changed in `config.h` and then recompile to apply.
There is also a xresources patch from [herbe](https://github.com/dudik/herbe)
that you could try to apply if you like.

`popup` can also be used as a standalone program to show the contents of stdin.
