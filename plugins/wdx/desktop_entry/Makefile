CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs glib-2.0 gio-unix-2.0` -I../../sdk
PLUGNAME = desktop_entry.wdx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
