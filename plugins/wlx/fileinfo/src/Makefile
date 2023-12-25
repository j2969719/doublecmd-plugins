CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed

libs := `pkg-config --cflags --libs gtk+-2.0`
includes := -I../../../sdk

srcvw2_libs := `pkg-config --cflags --libs gtk+-2.0 gtksourceview-2.0`

plugdir := $(shell basename $(realpath ..))
plugtype := $(shell basename $(realpath ../..))
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring :=


all: gtk2 srcvw2

gtk2:
		$(CC) $(CFLAGS) -o ../$(plugname) fileinfo.c $(libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' || echo $(plugname) >> ../../../dist/.build_fail.lst

srcvw2:
		$(CC) $(CFLAGS) -o ../$(plugdir)_srcvw.$(plugtype) fileinfo.c $(srcvw2_libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' -DWLXSRCVW || echo $(plugdir)_srcvw.$(plugtype) >> ../../../dist/.build_fail.lst


dist:
		test -f ../$(plugname) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../$(plugname) $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

		test -f ../$(plugdir)_srcvw.$(plugtype) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugdir)_srcvw.$(plugtype)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_srcvw_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../$(plugdir)_srcvw.$(plugtype) $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugdir)_srcvw.$(plugtype) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
