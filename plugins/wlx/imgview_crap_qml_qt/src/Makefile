CXX = g++
CXXFLAGS = -shared -fPIC -Wl,--no-as-needed

libs :=
includes := -I../../../sdk

qt5_libs := `pkg-config --cflags --libs Qt5Widgets Qt5QuickWidgets`
qt6_libs := `pkg-config --cflags --libs Qt6Widgets Qt6QuickWidgets`

plugdir := $(shell basename '$(realpath ..)')
plugtype := $(shell basename '$(realpath ../..)')
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype) ../*.qml), $(wildcard ../*))

detectstring := EXT=\"GIF\"|EXT=\"JPG\"|EXT=\"PNG\"


all: qt5 qt6

qt5:
		$(CXX) $(CXXFLAGS) -o '../$(plugdir)_$@.$(plugtype)' plugin.cpp $(libs) $($@_libs) $(includes) -D'PLUGNAME="'$(plugdir)_$@.$(plugtype)'"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' || echo '$(plugdir)_$@.$(plugtype)' >> ../../../dist/.build_fail.lst

qt6:
		$(CXX) $(CXXFLAGS) -o '../$(plugdir)_$@.$(plugtype)' plugin.cpp $(libs) $($@_libs) $(includes) -D'PLUGNAME="'$(plugdir)_$@.$(plugtype)'"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' || echo '$(plugdir)_$@.$(plugtype)' >> ../../../dist/.build_fail.lst

dist:
		test -f '../$(plugdir)_qt5.$(plugtype)' && test -f ../plugin_qt5.qml && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugdir)_qt5.$(plugtype)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_qt5_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../plugin_qt5.qml '../$(plugdir)_qt5.$(plugtype)' $(plugfiles) && \
			rm ../pluginst.inf || echo "$(plugdir)_qt5.$(plugtype) + plugin_qt5.qml" >> ../../../dist/.missing.log

		test -f '../$(plugdir)_qt6.$(plugtype)' && test -f ../plugin_qt6.qml && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugdir)_qt6.$(plugtype)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_qt6_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../plugin_qt6.qml '../$(plugdir)_qt6.$(plugtype)' $(plugfiles) && \
			rm ../pluginst.inf || echo "$(plugdir)_qt6.$(plugtype) + plugin_qt6.qml" >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
