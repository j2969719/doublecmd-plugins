CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -std=gnu99

libs := `pkg-config --cflags --libs glib-2.0 gio-2.0 gdk-pixbuf-2.0`
includes := -I../../../sdk

plugdir := $(shell basename '$(realpath ..)')
plugtype := $(shell basename '$(realpath ../..)')
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))


all: default tmppanel

default:
		$(CC) $(CFLAGS) -o '../$(plugname)' plugin.c $(libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'PLUGTARGET="$@"' || echo $(plugname) >> ../../../dist/.build_fail.lst

tmppanel:
		$(CC) $(CFLAGS) -o '../tp_$(plugname)' plugin.c $(libs) $(includes) -D'PLUGNAME="$(plugname)"' -D TEMP_PANEL -D'PLUGTARGET="$@"' || echo tp_$(plugname) >> ../../../dist/.build_fail.lst

dist:
		test -f '../$(plugname)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugname)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

		test -f '../tp_$(plugname)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=tp_$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_tmppanel_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../tp_$(plugname)' $(plugfiles) && \
			rm ../pluginst.inf || echo tp_$(plugname) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
