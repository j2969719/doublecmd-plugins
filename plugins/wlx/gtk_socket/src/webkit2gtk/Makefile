CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs webkit2gtk-4.0`


all:
		$(CC) $(CFLAGS) $(INCLUDES) kostyl.c -o ../../scripts/$(shell basename $(realpath .))/kostyl

clean:
		$(RM) ../../scripts/$(shell basename $(realpath .))/kostyl
