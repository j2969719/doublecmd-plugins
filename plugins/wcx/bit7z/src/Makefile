CXX = g++
CXXFLAGS = -shared -fPIC -Wl,--no-as-needed

libs := -L third_party/bit7z/lib/x64 -lbit7z64
includes := -I../../../sdk -Ithird_party/bit7z/include

plugdir := $(shell basename $(realpath ..))
plugtype := $(shell basename $(realpath ../..))
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

wcx_ext := 7z


all: default auto

default:
		$(CXX) $(CXXFLAGS) -o ../$(plugname) plugin.cpp $(libs) $(includes) `pkg-config --cflags --libs glib-2.0` -D'PLUGNAME="$(plugname)"'

auto:
		$(CXX) $(CXXFLAGS) -o ../auto_$(plugname) plugin.cpp -DBIT7Z_AUTO_FORMAT $(libs) $(includes) -D'PLUGNAME="auto_$(plugname)"'

dist:
		test -f ../$(plugname) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)\ndefaultextension=$(wcx_ext)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../$(plugname) $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

		test -f ../auto_$(plugname) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=auto_$(plugname)\ndefaultdir=$(plugdir)\ndefaultextension=cab" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_auto_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../auto_$(plugname) $(plugfiles) && \
			rm ../pluginst.inf || echo auto_$(plugname) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
