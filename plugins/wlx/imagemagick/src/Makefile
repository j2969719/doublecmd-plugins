CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed

libs := `pkg-config --cflags --libs gtkimageview`
includes := -I../../../sdk

libs_magick6 := `pkg-config --cflags --libs MagickWand`
libs_magick7 := `pkg-config --cflags --libs MagickWand`

plugdir := $(shell basename $(realpath ..))
plugtype := $(shell basename $(realpath ../..))
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring :=


all: 6 7
		test -f ../$(plugname) || echo $(plugname) >> ../../../dist/.build_fail.lst

6:
		$(CC) $(CFLAGS) -o ../$(plugname) wlximagemagick.c $(libs) $(libs_magick$@) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET=$@' || true
7:
		$(CC) $(CFLAGS) -o ../$(plugname) wlximagemagick.c $(libs) $(libs_magick$@) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET=$@' || true

dist:
		test -f ../$(plugname) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../$(plugname) $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
