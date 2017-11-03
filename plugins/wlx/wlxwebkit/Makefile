CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs gtk+-2.0 webkit-1.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) wlxwebkit.c -o wlxwebkit.wlx

clean:
		$(RM) wlxwebkit.wlx
