CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtk+-2.0` -I../../sdk  -lmagic -I/usr/include/magic

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o gtk_socket.wlx

clean:
		$(RM) gtk_socket.wlx
