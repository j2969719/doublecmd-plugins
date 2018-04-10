CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs gtk+-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o gtk_socket.wlx

clean:
		$(RM) gtk_socket.wlx
