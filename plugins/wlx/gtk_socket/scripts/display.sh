#!/bin/bash
xid=$1
file=$2
display -backdrop -foreground white -window $xid -window-group $xid "$file"