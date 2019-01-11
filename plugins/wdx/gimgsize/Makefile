CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs glib-2.0 gdk-2.0` -I../../sdk
PLUGNAME = gimgsize.wdx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
