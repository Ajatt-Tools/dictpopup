# dictpopup

This is a very lightweight program to show a popup with the translation of the selected text. It uses [sdcv](https://github.com/Dushistov/sdcv) for the dictionary lookup and a slightly modified version of [herbe](https://github.com/dudik/herbe) to display the popup.

![image](https://github.com/GenjiFujimoto/dictpopup/assets/50422430/c4a3663b-fd91-4a66-95ad-f1528071c932)

## Dependencies
xclip, sdcv

## Setup
First setup [sdcv](https://github.com/Dushistov/sdcv) according to their github page.
Then compile with `make` and place `dictpopup` as well as `popup` in your PATH.

## Usage
call `dictpopup` to translate and display the popup of the selected text. The
popup can be dismissed by clicking on it.
The styling can be changed in `config.h`, but it has the be recompiled then.
There is also a xresources patch from [herbe](https://github.com/dudik/herbe)
that you could try to apply.

You can also use `popup` like `popup "Hello"` for other things if you want.
