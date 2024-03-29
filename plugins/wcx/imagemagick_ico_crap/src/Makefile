CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed

libs := `pkg-config --cflags --libs glib-2.0`
includes := -I../../../sdk

libs_magick6 := `pkg-config --cflags --libs MagickWand`
libs_magick7 := `pkg-config --cflags --libs MagickWand`

plugdir := $(shell basename '$(realpath ..)')
plugtype := $(shell basename '$(realpath ../..)')
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

wcx_ext := ico


all: 6 7
		test -f '../$(plugname)' || echo $(plugname) >> ../../../dist/.build_fail.lst

6:
		$(CC) $(CFLAGS) -o '../$(plugname)' plugin.c $(libs) $(libs_magick$@) $(includes) -D'PLUGNAME="$(plugname)"' -D'PLUGTARGET=$@' || true
7:
		$(CC) $(CFLAGS) -o '../$(plugname)' plugin.c $(libs) $(libs_magick$@) $(includes) -D'PLUGNAME="$(plugname)"' -D'PLUGTARGET=$@' || true

dist:
		test -f '../$(plugname)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)\ndefaultextension=$(wcx_ext)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugname)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
