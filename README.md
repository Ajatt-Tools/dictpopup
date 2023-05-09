# dictpopup

It's a very lightweight program to show a popup with the translation of the selected text. It uses [sdcv](https://github.com/Dushistov/sdcv) for the dictionary lookup and a slightly modified version of [herbe](https://github.com/dudik/herbe) to display the popup.

## Dependencies
xclip, sdcv

## Setup
Compile with `make` and place `dictpopup` as well as `popup` in your PATH.

## Usage
call `dictpopup` to translate and display the popup of the selected text. 
You can also use `popup` like `popup "Hello"` for other things.
