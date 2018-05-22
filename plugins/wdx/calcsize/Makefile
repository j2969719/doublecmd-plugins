CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) calcsize.c -o calcsize.wdx

clean:
		$(RM) calcsize.wdx
