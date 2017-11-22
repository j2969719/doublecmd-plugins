CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs abiword-3.0` -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) wlxabiword.c -o wlxabiword.wlx

clean:
		$(RM) wlxabiword.wlx
