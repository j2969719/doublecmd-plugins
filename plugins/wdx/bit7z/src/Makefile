CXX = g++
CXXFLAGS = -shared -fPIC -Wl,--no-as-needed

libs := -L third_party/bit7z/lib/x64 -lbit7z64
includes := -I../../../sdk -Ithird_party/bit7z/include

plugdir := $(shell basename $(realpath ..))
plugtype := $(shell basename $(realpath ../..))
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring :=


all:
		$(CXX) $(CXXFLAGS) -o ../$(plugname) plugin.cpp -DBIT7Z_AUTO_FORMAT $(libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"'

dist:
		test -f ../$(plugname) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../$(plugname) $(plugfiles) && \
			rm ../pluginst.inf || echo turboencabulator

clean:
		$(RM) $(wildcard ../*.$(plugtype))
