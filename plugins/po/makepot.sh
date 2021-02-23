#!/bin/sh

find .. \( -name "*.c" -o -name "*.cpp" \) -type f -print0 | xargs -0 xgettext -k_ -kN_ --language=C --output=plugins.pot -