CC = gcc
CFLAGS = -shared -fPIC -Wall -pedantic -g

libs := `pkg-config --cflags --libs gio-2.0 speech-dispatcher`
includes := -I../../sdk
libname = $(shell basename $(realpath .))

#all: lua luajit lua5.1 lua5.2 lua5.3 lua5.4 lua5.5
all: lua luajit

lua:
		$(CC) $(CFLAGS) -o ../$(libname).so module.c $(libs) $(includes) `pkg-config --libs --cflags lua` -DLUAOPEN_NAME=luaopen_$(libname)

luajit:
		$(CC) $(CFLAGS) -o ../$(libname)_$@.so module.c $(libs) $(includes) `pkg-config --libs --cflags luajit` -DLUAOPEN_NAME=luaopen_$(libname)_$@ -DPSHANICA

lua5.1:
		$(CC) $(CFLAGS) -o ../$(libname)_$@.so module.c $(libs) $(includes) `pkg-config --libs --cflags lua5.1` -DLUAOPEN_NAME=luaopen_$(libname)_$@

lua5.2:
		$(CC) $(CFLAGS) -o ../$(libname)_$@.so module.c $(libs) $(includes) `pkg-config --libs --cflags lua5.2` -DLUAOPEN_NAME=luaopen_$(libname)_$@

lua5.3:
		$(CC) $(CFLAGS) -o ../$(libname)_$@.so module.c $(libs) $(includes) `pkg-config --libs --cflags lua5.3` -DLUAOPEN_NAME=luaopen_$(libname)_$@

lua5.4:
		$(CC) $(CFLAGS) -o ../$(libname)_$@.so module.c $(libs) $(includes) `pkg-config --libs --cflags lua5.4` -DLUAOPEN_NAME=luaopen_$(libname)_$@

lua5.5:
		$(CC) $(CFLAGS) -o ../$(libname)_$@.so module.c $(libs) $(includes) `pkg-config --libs --cflags lua5.5` -DLUAOPEN_NAME=luaopen_$(libname)_$@

clean:
		$(RM) ../$(libname).so
		$(RM) ../$(libname)_*.so