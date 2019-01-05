CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs glib-2.0 gio-2.0` -I../../sdk
PLUGNAME = g_descr.wdx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
