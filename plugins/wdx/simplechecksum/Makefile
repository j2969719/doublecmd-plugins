CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -lgcrypt -lgpg-error -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) simplechecksumwdx.c -o simplechecksum.wdx

clean:
		$(RM) simplechecksum.wdx
