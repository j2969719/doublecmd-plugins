CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs gtk+-3.0` -llibreofficekitgtk -I.

all:
		$(CC) $(CFLAGS) $(INCLUDES) kostyl.c -o kostyl

clean:
		$(RM) kostyl
