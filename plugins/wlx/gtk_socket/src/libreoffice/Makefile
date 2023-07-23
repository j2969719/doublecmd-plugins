CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs gtk+-3.0` -llibreofficekitgtk -I.

all:
		$(CC) $(CFLAGS) $(INCLUDES) kostyl.c -o ../../scripts/$(shell basename $(realpath .))/kostyl

clean:
		$(RM) ../../scripts/$(shell basename $(realpath .))/kostyl
