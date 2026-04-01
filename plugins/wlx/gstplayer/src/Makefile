CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -Wall -O2 -Wno-deprecated-declarations

libs := `pkg-config --cflags --libs gstreamer-video-1.0 gstreamer-1.0`
includes := -I../../../sdk

gtk2_libs := `pkg-config --cflags --libs gtk+-2.0`
gtk3_libs := `pkg-config --cflags --libs gtk+-3.0`


plugdir := $(shell basename '$(realpath ..)')
plugtype := $(shell basename '$(realpath ../..)')
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring := EXT=\"AVI\"|EXT=\"MKV\"|EXT=\"FLV\"|EXT=\"MPG\"|EXT=\"MPEG\"|EXT=\"MP4\"|EXT=\"3GP\"|EXT=\"MP3\"|EXT=\"OGG\"|EXT=\"WMA\"|EXT=\"BIK\"|EXT=\"VOC\"|EXT=\"WAV\"|EXT=\"WEBM\"|EXT=\"VOB\"|EXT=\"ROQ\"|EXT=\"IVF\"|EXT=\"MOV\"|EXT=\"FLAC\"|EXT=\"WMV\"|EXT=\"M4A\"|EXT=\"AIF\"|EXT=\"AIFF\"|EXT=\"OPUS\"


all: gtk2 gtk3

gtk2:
		$(CC) $(CFLAGS) -o '../$(plugname)' gstplayer.c $(libs) $($@_libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' || echo $(plugname) >> ../../../dist/.build_fail.lst
gtk3:
		$(CC) $(CFLAGS) -o '../$(plugdir)_$@.$(plugtype)' gstplayer.c $(libs) $($@_libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -DGTK3PLUG -D'PLUGTARGET="$@"' #|| echo $(plugname) >> ../../../dist/.build_fail.lst

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
