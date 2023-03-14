CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs glib-2.0 gio-2.0 taglib_c` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
