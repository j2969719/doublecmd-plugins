CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs glib-2.0 gio-2.0` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))


all:
		$(CC) $(CFLAGS) $(INCLUDES) datetimestr.c -o ../$(PLUGNAME) -D'PLUGNAME="$(PLUGNAME)"'

clean:
		$(RM) ../$(PLUGNAME)
