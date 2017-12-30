CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --libs --cflags gtk+-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) fileinfo.c -o fileinfo.wlx

clean:
		$(RM) fileinfo.wlx
