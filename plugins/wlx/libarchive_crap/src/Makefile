CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtk+-2.0 libarchive` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
