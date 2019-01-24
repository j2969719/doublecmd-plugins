CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --libs --cflags gtk+-2.0 gtksourceview-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) mimescriptwlx.c -o mimescript.wlx

clean:
		$(RM) mimescript.wlx
