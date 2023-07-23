CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed

libs := `pkg-config --cflags --libs gtk+-2.0`
includes := -I../../../sdk

plugdir := $(shell basename $(realpath ..))
plugtype := $(shell basename $(realpath ../..))
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))


all:
		$(CC) $(CFLAGS) -o ../$(plugname) plugin.c $(libs) $(includes) -D'PLUGNAME="$(plugname)"'

dist:
		# nobody cares about search plugins:(

clean:
		$(RM) $(wildcard ../*.$(plugtype))
