CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --libs --cflags glib-2.0 gtk+-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) clip.c -o clip.wfx

clean:
		$(RM) clip.wfx
