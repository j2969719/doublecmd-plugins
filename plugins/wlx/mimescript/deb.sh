#!/bin/bash
path=$1
filemane=$(basename $path)
tmp=`mktemp -d ${TMPDIR:-/tmp}/deb_XXXXXX`
file -b "$1"
cp "$path" "$tmp"
cd "$tmp"
ar -x "$filemane"
tar -xf control.tar.gz ./control ./md5sums -O | fmt --width=80 -s
echo
tar -tvf data.tar.xz
tar -tvf data.tar.gz
rm -rf "$tmp"