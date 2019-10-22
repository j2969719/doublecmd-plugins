CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs glib-2.0 gio-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o gvfs_quickmount.wfx

clean:
		$(RM) gvfs_quickmount.wfx
