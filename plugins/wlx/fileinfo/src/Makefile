CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --libs --cflags gtk+-2.0 gtksourceview-2.0` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) fileinfo.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
