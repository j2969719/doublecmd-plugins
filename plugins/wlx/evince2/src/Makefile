CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed

libs := `pkg-config --cflags --libs gtk+-2.0 evince2-view-2.32 evince2-document-2.32`
includes := -I../../../sdk

plugdir := $(shell basename $(realpath ..))
plugtype := $(shell basename $(realpath ../..))
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring :=


all:
		$(CC) $(CFLAGS) -o ../$(plugname) wlxevince.c $(libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"'

dist:
		test -f ../$(plugname) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../$(plugname) $(plugfiles) && \
			rm ../pluginst.inf || echo turboencabulator

clean:
		$(RM) $(wildcard ../*.$(plugtype))
