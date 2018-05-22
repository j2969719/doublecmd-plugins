CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) emptydir.c -o emptydir.wdx

clean:
		$(RM) emptydir.wdx
