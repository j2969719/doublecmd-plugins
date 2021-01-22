CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -lmagic -I/usr/include/magic -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) simplefileinfowdx.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
