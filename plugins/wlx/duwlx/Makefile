CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --libs --cflags gtk+-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) duwlx.c -o du.wlx

clean:
		$(RM) du.wlx
