#!/bin/sh

echo missing libs: > dist/.broken.log
find $PWD -type f -name "*.[dw][cdfls]x" -not -path "*/third_party/*" -exec sh -c "ldd '{}' | grep 'not found' && echo '{}' >> $PWD/dist/.broken.log" \;
echo "END" >> dist/.broken.log