CXX = g++
CXXFLAGS = -shared -fPIC -Wl,--no-as-needed

libs :=
includes := -I../../../sdk

qt5_libs := `pkg-config --cflags --libs Qt5Widgets libarchive`

plugdir := $(shell basename $(realpath ..))
plugtype := $(shell basename $(realpath ../..))
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring :=


all: qt5

qt5:
		$(CXX) $(CXXFLAGS) -o ../$(plugdir)_$@.$(plugtype) plugin.cpp $(libs) $($@_libs) $(includes) -D'PLUGNAME="$(plugdir)_$@.$(plugtype)"' -D'DETECT_STRING="$(detectstring)"'

dist:
		test -f ../$(plugdir)_qt5.$(plugtype) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugdir)_qt5.$(plugtype)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_qt5_$(shell date +%y.%m.%d).tar.gz' ../$(plugdir)_qt5.$(plugtype) $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugdir)_qt5.$(plugtype) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
