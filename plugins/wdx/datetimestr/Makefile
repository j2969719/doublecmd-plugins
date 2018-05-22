CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs glib-2.0 gio-2.0` -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) datetimestr.c -o datetimestr.wdx

clean:
		$(RM) datetimestr.wdx
