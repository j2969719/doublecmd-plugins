#!/bin/sh

file=$1
which xlhtml >/dev/null 2>&1 && {
	tmp=`mktemp -d ${TMPDIR:-/tmp}/%p.XXXXXX`
	xlhtml -a "$file" > "$tmp/page.html"
	elinks -dump "$tmp/page.html"
	rm -rf "$tmp"
} || \
xls2csv "$file" | sed -e 's/,,/, ,/g' | column -s, -t || \
strings "$file"