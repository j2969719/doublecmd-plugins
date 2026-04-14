CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -Wall -pedantic -g

libs := `pkg-config --cflags --libs gio-2.0 gio-unix-2.0`
includes := -I../../../sdk

gtk2_libs := `pkg-config --cflags --libs gtk+-2.0` -Wno-deprecated-declarations
gtk3_libs := `pkg-config --cflags --libs gtk+-3.0`


plugdir := $(shell basename '$(realpath ..)')
plugtype := $(shell basename '$(realpath ../..)')
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring :=


all: gtk2 gtk3

gtk2:
		$(CC) $(CFLAGS) -o '../$(plugname)' plugin.c $(libs) $($@_libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGDIR="$(plugdir)"' || echo $(plugname) >> ../../../dist/.build_fail.lst
gtk3:
		$(CC) $(CFLAGS) -o '../$(plugdir)_$@.$(plugtype)' plugin.c $(libs) $($@_libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -DGTK3PLUG -D'PLUGDIR="$(plugdir)"' || echo $(plugname) >> ../../../dist/.build_fail.lst

dist:
		test -f '../$(plugname)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugname)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

		test -f '../$(plugdir)_gtk3.$(plugtype)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugdir)_gtk3.$(plugtype)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_gtk3_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugdir)_gtk3.$(plugtype)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
