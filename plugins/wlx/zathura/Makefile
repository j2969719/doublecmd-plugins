CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs gtk+-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) wlxzathura.c -o zathura.wlx

clean:
		$(RM) zathura.wlx
