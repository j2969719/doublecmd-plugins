CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -lgcrypt -lgpg-error -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) simplechecksumwdx.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
