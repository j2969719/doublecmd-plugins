CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --libs --cflags gtk+-2.0 gtksourceview-2.0` -lmagic -I/usr/include/magic -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) mimescriptwlx.c -o mimescript.wlx

clean:
		$(RM) mimescript.wlx
