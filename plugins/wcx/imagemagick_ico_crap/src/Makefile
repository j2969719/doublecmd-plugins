CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -Wall -pedantic
INCLUDES = `pkg-config --cflags --libs glib-2.0 MagickWand` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
