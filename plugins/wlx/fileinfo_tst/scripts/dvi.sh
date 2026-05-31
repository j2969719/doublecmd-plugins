#!/bin/sh

file=$1
which dvi2tty >/dev/null 2>&1 && \
	dvi2tty "$file" || \
catdvi "$file"