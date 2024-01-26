CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -Wall -O2

libs := `pkg-config --cflags --libs gstreamer-video-1.0 gtk+-2.0 gstreamer-1.0`
includes := -I../../../sdk

plugdir := $(shell basename '$(realpath ..)')
plugtype := $(shell basename '$(realpath ../..)')
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring := (EXT=\"AVI\")|(EXT=\"MKV\")|(EXT=\"FLV\")|(EXT=\"MPG\")|(EXT=\"MPEG\")|(EXT=\"MP4\")|(EXT=\"3GP\")|(EXT=\"MP3\")|(EXT=\"OGG\")|(EXT=\"WMA\")|(EXT=\"BIK\")|(EXT=\"VOC\")|(EXT=\"WAV\")|(EXT=\"WEBM\")|(EXT=\"VOB\")|(EXT=\"ROQ\")|(EXT=\"IVF\")|(EXT=\"MOV\")|(EXT=\"FLAC\")|(EXT=\"WMV\")|(EXT=\"M4A\")|(EXT=\"AIF\")|(EXT=\"AIFF\")|(EXT=\"OPUS\")


all:
		$(CC) $(CFLAGS) -o '../$(plugname)' gstplayer.c $(libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' || echo $(plugname) >> ../../../dist/.build_fail.lst

dist:
		test -f '../$(plugname)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugname)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
