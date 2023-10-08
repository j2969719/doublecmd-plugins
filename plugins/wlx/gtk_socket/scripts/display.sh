#!/bin/bash
xid=$1
file=$2
# Debian/Ubuntu:  Check the installed ImageMagick packages.
# If the Q16HDRI version is installed, then add the "imagemagick-6.q16hdri"
# package and use "display-im6.q16hdri" instead of "display". Otherwise, add
# the "imagemagick-6.q16" package and use "display-im6.q16" instead of "display".
display -backdrop -foreground white -window $xid -window-group $xid "$file"
