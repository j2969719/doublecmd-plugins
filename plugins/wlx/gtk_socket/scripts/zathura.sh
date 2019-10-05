#!/bin/bash
xid=$1
file=$2
script_dir="`dirname "$0"`"

export GDK_CORE_DEVICE_EVENTS=1
# zathura --reparent=$xid $file --page=1
# zathura --reparent=$xid $file --page=1 --config-dir=$script_dir --data-dir=$script_dir
zathura --reparent=$xid $file --page=1