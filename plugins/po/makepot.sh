#!/bin/sh

find .. -name "*.c" | xargs xgettext -k_ -kN_ --language=C --output=plugins.pot -