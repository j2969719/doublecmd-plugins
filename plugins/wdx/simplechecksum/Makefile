CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs zlib openssl` -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) simplechecksumwdx.c -o simplechecksum.wdx

clean:
		$(RM) simplechecksum.wdx
