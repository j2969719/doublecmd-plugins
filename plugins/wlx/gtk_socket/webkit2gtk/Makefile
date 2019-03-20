CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs webkit2gtk-4.0`


all:
		$(CC) $(CFLAGS) $(INCLUDES) kostyl.c -o kostyl

clean:
		$(RM) kostyl
