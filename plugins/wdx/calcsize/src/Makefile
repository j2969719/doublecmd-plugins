CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))


all:
		$(CC) $(CFLAGS) $(INCLUDES) calcsize.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
