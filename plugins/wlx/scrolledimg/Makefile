CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs gtk+-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) scrolledimg.c -o scrolledimg.wlx

clean:
		$(RM) scrolledimg.wlx
