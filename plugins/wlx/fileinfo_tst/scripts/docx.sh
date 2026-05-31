#!/bin/sh

file=$1
docx2txt "$file" - || \
unzip -p "$file" | grep --text '<w:r' | sed 's/<w:p[^<\/]*>/ \
	/g' | sed 's/<[^<]*>//g' | grep -v '^[[:space:]]*$' | sed G