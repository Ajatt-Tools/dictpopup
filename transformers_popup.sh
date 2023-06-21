#!/bin/sh

transformers_ocr recognize || exit 1
clipnotify -s clipboard && dictpopup "$(xclip -o -sel clip)"
