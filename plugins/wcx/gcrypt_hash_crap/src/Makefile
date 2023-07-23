CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -std=gnu99

libs := -lgcrypt -lgpg-error `pkg-config --cflags --libs glib-2.0`
includes := -I../../../sdk

plugdir := $(shell basename $(realpath ..))
plugtype := $(shell basename $(realpath ../..))
plugname := $(plugdir).$(plugtype)
plugdescr := `grep '$(plugtype)/$(plugdir))' ../../../../plugins.md -1 | tail -1 | sed 's/[[]//' | sed 's/[]][\(][^\)]\+.//' | sed 's/\s[\(].\+[\)]//'`
plugfiles := $(filter-out $(wildcard ../*.$(plugtype)), $(wildcard ../*))

wcx_ext := blake2b_512,blake2b_384,blake2b_256,blake2b_160,blake2s_256,blake2s_224,blake2s_160,blake2s_128,crc24_rfc2440,crc32,crc32_rfc1510,gostr3411_94,gostr3411_cp,haval,md2,md4,md5,rmd160,sha1,sha224,sha256,sha384,sha512,sha512_224,sha512_256,sha3_224,sha3_256,sha3_384,sha3_512,sm3,stribog256,stribog512,tiger,tiger1,tiger2,whirlpool


all:
		$(CC) $(CFLAGS) -o ../$(plugname) plugin.c $(libs) $(includes) -D'PLUGNAME="$(plugname)"'

dist:
		test -f ../$(plugname) && \
			echo -e "[plugininstall]\ndescription=$(plugdescr)\ntype=$(plugtype)\nfile=$(plugname)\ndefaultdir=$(plugdir)\ndefaultextension=$(wcx_ext)" > ../pluginst.inf && \
			tar --exclude=../src -h -cvzf '../../../dist/$(plugtype)_$(plugdir)_$(shell date +%y.%m.%d).tar.gz' ../pluginst.inf ../$(plugname) $(plugfiles) && \
			rm ../pluginst.inf || echo $(plugname) >> ../../../dist/.missing.log

clean:
		$(RM) $(wildcard ../*.$(plugtype))
