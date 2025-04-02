CC = gcc
CXX = g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed -Wno-deprecated-declarations
CXXFLAGS = -shared -fPIC -Wl,--no-as-needed

libs :=
includes := -I../../../sdk

gtk2_libs := `pkg-config --cflags --libs gtk+-2.0`
qt5_libs := `pkg-config --cflags --libs Qt5Widgets`
qt6_libs := `pkg-config --cflags --libs Qt6Widgets Qt6Core Qt6Gui`

plugdir := $(shell basename '$(realpath ..)')
plugtype := $(shell basename '$(realpath ../..)')
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring := (EXT=\"AVI\")|(EXT=\"MKV\")|(EXT=\"FLV\")|(EXT=\"MPG\")|(EXT=\"MPEG\")|(EXT=\"MP4\")|(EXT=\"3GP\")|(EXT=\"MP3\")|(EXT=\"OGG\")|(EXT=\"WMA\")|(EXT=\"BIK\")|(EXT=\"VOC\")|(EXT=\"WAV\")|(EXT=\"WEBM\")|(EXT=\"VOB\")|(EXT=\"ROQ\")|(EXT=\"IVF\")|(EXT=\"MOV\")|(EXT=\"FLAC\")|(EXT=\"WMV\")|(EXT=\"M4A\")|(EXT=\"AIF\")|(EXT=\"AIFF\")|(EXT=\"OPUS\")


all: gtk2 qt5 qt6

gtk2:
		$(CC) $(CFLAGS) -o '../$(plugdir)_$@.$(plugtype)' plugin.c $(libs) $($@_libs) $(includes) -D'PLUGNAME="'$(plugdir)_$@.$(plugtype)'"' -D'DETECT_STRING="$(detectstring)"' -DGTKPLUG || echo '$(plugdir)_$@.$(plugtype)' >> ../../../dist/.build_fail.lst

qt5:
		$(CXX) $(CXXFLAGS) -o '../$(plugdir)_$@.$(plugtype)' plugin.c $(libs) $($@_libs) $(includes) -D'PLUGNAME="'$(plugdir)_$@.$(plugtype)'"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' || echo '$(plugdir)_$@.$(plugtype)' >> ../../../dist/.build_fail.lst

qt6:
		$(CXX) $(CXXFLAGS) -o '../$(plugdir)_$@.$(plugtype)' plugin.c $(libs) $($@_libs) $(includes) -D'PLUGNAME="'$(plugdir)_$@.$(plugtype)'"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' || echo '$(plugdir)_$@.$(plugtype)' >> ../../../dist/.build_fail.lst

dist:
		test -f '../$(plugdir)_gtk2.$(plugtype)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugdir)_gtk2.$(plugtype)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_gtk2_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugdir)_gtk2.$(plugtype)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugdir)_gtk2.$(plugtype) >> ../../../dist/.missing.log


		test -f '../$(plugdir)_qt5.$(plugtype)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugdir)_qt5.$(plugtype)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_qt5_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugdir)_qt5.$(plugtype)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugdir)_qt5.$(plugtype) >> ../../../dist/.missing.log

		test -f '../$(plugdir)_qt6.$(plugtype)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugdir)_qt6.$(plugtype)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_qt6_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugdir)_qt6.$(plugtype)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugdir)_qt6.$(plugtype) >> ../../../dist/.missing.log
clean:
		$(RM) $(wildcard ../*.$(plugtype))
