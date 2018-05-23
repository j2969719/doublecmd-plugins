CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --libs --cflags glib-2.0 gtk+-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) envlistwfx.c -o envlist.wfx

clean:
		$(RM) envlist.wfx
