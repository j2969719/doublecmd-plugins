#!/bin/sh

find $PWD -type f -name "Makefile" -not -path "*/third_party/*" -exec sh -c 'cd `dirname $0` && make clean' '{}' \;