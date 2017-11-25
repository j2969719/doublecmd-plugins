CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -D_GNU_SOURCE
INCLUDES = `pkg-config --cflags --libs gtksourceview-2.0 libconfig` -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) gtksourceview.c -o gtksourceview.wlx

clean:
		$(RM) gtksourceview.wlx
