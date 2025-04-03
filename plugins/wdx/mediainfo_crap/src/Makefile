CXX = g++
CXXFLAGS = -shared -fPIC -Wl,--no-as-needed

libs := `pkg-config --cflags --libs libmediainfo`
includes := -I../../../sdk -I.

plugdir := $(shell basename '$(realpath ..)')
plugtype := $(shell basename '$(realpath ../..)')
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring := (EXT=\"3G2\")|(EXT=\"3GP\")|(EXT=\"ASF\")|(EXT=\"AVI\")|(EXT=\"DIVX\")|(EXT=\"FLV\")|(EXT=\"M2T\")|(EXT=\"M2TS\")|(EXT=\"M2V\")|(EXT=\"M4V\")|(EXT=\"MKV\")|(EXT=\"MOV\")|(EXT=\"MP4\")|(EXT=\"MPEG\")|(EXT=\"MPG\")|(EXT=\"MTS\")|(EXT=\"OGG\")|(EXT=\"TS\")|(EXT=\"VOB\")|(EXT=\"WEBM\")|(EXT=\"WMV\")


all:
		$(CXX) $(CXXFLAGS) -o '../$(plugname)' plugin.cpp $(libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' || echo $(plugname) >> ../../../dist/.build_fail.lst

dist:
		test -f '../$(plugname)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugname)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
