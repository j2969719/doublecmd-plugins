CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -g -Wall -pedantic

libs := `pkg-config --cflags --libs gtk+-3.0` -llibreofficekitgtk
includes := -I../../../sdk  -I.

plugdir := $(shell basename '$(realpath ..)')
plugtype := $(shell basename '$(realpath ../..)')
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

detectstring := EXT=\"ODT\"|EXT=\"DOC\"|EXT=\"DOCX\"|EXT=\"DOCM\"|EXT=\"DOT\"|EXT=\"DOTX\"|EXT=\"RTF\"|EXT=\"EPUB\"|EXT=\"WPD\"|EXT=\"PAGES\"|EXT=\"ODS\"|EXT=\"XLS\"|EXT=\"XLSX\"|EXT=\"XLSM\"|EXT=\"XLT\"|EXT=\"XLTX\"|EXT=\"CSV\"|EXT=\"NUMBERS\"|EXT=\"ODP\"|EXT=\"PPT\"|EXT=\"PPTX\"|EXT=\"ODG\"|EXT=\"VSD\"|EXT=\"VSDX\"|EXT=\"FODT\"|EXT=\"FODS\"|EXT=\"SXW\"|EXT=\"STW\"|EXT=\"FB2\"|EXT=\"ABW\"|EXT=\"UOT\"|EXT=\"UOP\"


all:
		$(CC) $(CFLAGS) -o '../$(plugname)' plugin.c $(libs) $(includes) -D'PLUGNAME="$(plugname)"' -D'DETECT_STRING="$(detectstring)"' -D'PLUGTARGET="$@"' || echo $(plugname) >> ../../../dist/.build_fail.lst

dist:
		test -f '../$(plugname)' && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf '../$(plugname)' $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
