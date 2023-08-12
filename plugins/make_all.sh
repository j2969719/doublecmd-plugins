#!/bin/sh

echo "build failed:" > dist/.build_fail.lst
find $PWD -type f -name "Makefile" -not -path "*/third_party/*" -exec sh -c 'cd `dirname $0` && make' '{}' \;