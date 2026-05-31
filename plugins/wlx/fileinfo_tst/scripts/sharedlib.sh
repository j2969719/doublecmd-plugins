#!/bin/sh

file=$1
file -b "$file"
ldd  "$1" | grep "not found"
echo
readelf -a "$file"
nm -CD "$file"