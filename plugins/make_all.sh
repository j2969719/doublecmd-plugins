#!/bin/sh

find $PWD -type f -name "Makefile" -exec sh -c 'cd `dirname $0` && make' '{}' \;