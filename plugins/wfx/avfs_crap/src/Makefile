CC = gcc
CFLAGS = -shared -fPIC -O -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs avfs` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
