#!/bin/sh

echo missing binaries: > dist/.missing.log
find $PWD -type f -name "Makefile" -not -path "*/third_party/*" -exec sh -c 'cd `dirname $0` && make dist' '{}' \;

cat "`dirname $0`/dist/.plugins.md" | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//' | md2html --github > "`dirname $0`/dist/plugins.htm" && links "`dirname $0`/dist/plugins.htm" -dump > "`dirname $0`/dist/plugins.txt" && rm "`dirname $0`/dist/plugins.htm"