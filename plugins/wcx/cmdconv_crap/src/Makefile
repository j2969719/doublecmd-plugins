CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -I../../../sdk `pkg-config --cflags --libs glib-2.0`
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
