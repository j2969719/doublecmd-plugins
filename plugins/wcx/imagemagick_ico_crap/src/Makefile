CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed

libs := `pkg-config --cflags --libs glib-2.0 MagickWand`
includes := -I../../../sdk

plugdir := $(shell basename $(realpath ..))
plugtype := $(shell basename $(realpath ../..))
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

wcx_ext := ico


all:
		$(CC) $(CFLAGS) -o ../$(plugname) plugin.c $(libs) $(includes) -D'PLUGNAME="$(plugname)"'

dist:
		test -f ../$(plugname) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)\ndefaultextension=$(wcx_ext)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../$(plugname) $(plugfiles) && \
			rm ../pluginst.inf || echo turboencabulator

clean:
		$(RM) $(wildcard ../*.$(plugtype))
