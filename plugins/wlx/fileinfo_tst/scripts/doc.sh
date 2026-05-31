#!/bin/sh

file=$1
which wvHtml >/dev/null 2>&1 && {
	tmp=`mktemp -d ${TMPDIR:-/tmp}/%p.XXXXXX`
	wvHtml "$file" --targetdir="$tmp" page.html
	elinks -dump "$tmp/page.html"
	rm -rf "$tmp"
} || \
antiword -t "$file" || \
catdoc -w "$file" || \
word2x -f text "$file"