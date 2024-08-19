#!/bin/sh

adventuretime=`date "+%Y-%m-%d %T"`

echo "missing libs ($adventuretime):" > dist/.broken.log
find "$PWD" -type f -name "*.[dw][cdfls]x" -not -path "*/third_party/*" -exec sh -c "ldd '{}' | grep 'not found' && echo '{}' >> \"$PWD/dist/.broken.log\"" \;
echo "END ($adventuretime)" >> dist/.broken.log