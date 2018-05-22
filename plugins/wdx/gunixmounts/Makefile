CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs glib-2.0 gio-unix-2.0` -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) mounts.c -o mounts.wdx

clean:
		$(RM) mounts.wdx
