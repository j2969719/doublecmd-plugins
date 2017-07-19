#!/bin/bash
if (zenity --question \
--text "Empty trash?")
then (
echo "1" ; sleep 1
gvfs-trash --empty
echo "99" ; sleep 1
) |
zenity --progress \
  --auto-close \
  --title="Processing" \
  --percentage=0

fi