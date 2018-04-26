CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs gtk+-2.0` -lmagic -I/usr/include/magic -I../../sdk
PLUGNAME = gtk_socket_mime.wlx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
