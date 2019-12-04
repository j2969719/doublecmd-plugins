CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs libgcrypt` -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) simplechecksumwdx.c -o simplechecksum.wdx

clean:
		$(RM) simplechecksum.wdx
