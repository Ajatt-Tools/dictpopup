# dictpopup

This is a very lightweight program to show a popup with the translation of the selected text. It uses [sdcv](https://github.com/Dushistov/sdcv) for the dictionary lookup and a slightly modified version of [herbe](https://github.com/dudik/herbe) to display the popup.

![image](https://github.com/GenjiFujimoto/dictpopup/assets/50422430/c4a3663b-fd91-4a66-95ad-f1528071c932)

## Dependencies
sselp (can be replaced with xclip -o), sdcv. Optional: lynx for html support

## Setup
First setup [sdcv](https://github.com/Dushistov/sdcv) according to their github page.
Then install with `sudo make install`. \
Uninstall with `sudo make uninstall`

## Usage
Call `dictpopup [html] [<WORD>]`.\
If no word as an argument is supplied, the selection is used.\
If the string `html` is supplied as the first argument, then html support will be enabled.\
The popup can be dismissed by clicking on it.

The styling can be changed in `config.h` and then recompile to apply.
There is also a xresources patch from [herbe](https://github.com/dudik/herbe)
that you could try to apply if you like.

`popup` can also be used as a standalone program to show the contents of stdin.
