CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0` -lyelp

all:
		$(CC) $(CFLAGS) $(INCLUDES) kostyl.c -o kostyl

clean:
		$(RM) $(PLGNAME) kostyl
