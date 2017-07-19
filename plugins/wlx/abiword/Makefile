CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs abiword-3.0`


all:
		$(CC) $(CFLAGS) $(INCLUDES) wlxabiword.c -o wlxabiword.wlx

clean:
		$(RM) wlxabiword.wlx
