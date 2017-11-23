CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtksourceview-2.0` -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) gtksourceview.c -o gtksourceview.wlx

clean:
		$(RM) gtksourceview.wlx
