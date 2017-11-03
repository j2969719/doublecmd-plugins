CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = -lmagic -I/usr/include/magic -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) simplefileinfowdx.c -o simplefileinfo.wdx

clean:
		$(RM) simplefileinfo.wdx
