CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs gtk+-3.0` -llibreofficekitgtk -I.

all:
		$(CC) $(CFLAGS) kostyl.c $(INCLUDES) -o ../../$(shell basename $(realpath .))_kostyl

clean:
		$(RM) ../../$(shell basename $(realpath .))_kostyl
