CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -lphysfs -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
