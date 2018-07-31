#!/bin/bash
exiftool  "$1" || \
wrestool --extract --raw --type=version "$1" |  strings -el
echo
readpe -A "$1"


