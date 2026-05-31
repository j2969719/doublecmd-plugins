#!/bin/sh

file=$1
exiftool "$file" || wrestool --extract --raw --type=version "$file" | strings -el
readpe -A "$file"