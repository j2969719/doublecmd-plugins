#!/bin/sh

adventuretime=`date "+%Y-%m-%d %T"`

echo "build failed ($adventuretime):" > dist/.build_fail.lst
find "$PWD" -type f -name "Makefile" -not -path "*/third_party/*" -exec sh -c 'cd "`dirname "$0"`" && make' '{}' \;
echo "END ($adventuretime)" >> dist/.build_fail.lst