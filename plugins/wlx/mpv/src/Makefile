CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtk+-2.0` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) wlxmpv.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
