#! /bin/sh

if [ ! -d "$HOME/.FreeArc" ]; then
 	mkdir -p $HOME/.FreeArc
	cp --no-preserve=ownership /etc/FreeArc/* $HOME/.FreeArc
	cp --no-preserve=ownership /usr/lib/FreeArc/scripts/*.lua $HOME/.FreeArc/
fi

"/usr/lib/FreeArc/arc" "$@"
