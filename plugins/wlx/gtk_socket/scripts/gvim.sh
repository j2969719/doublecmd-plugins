#!/bin/bash
xid=$1
file=$2
gvim --servername $xid --socketid $xid "$file" -R