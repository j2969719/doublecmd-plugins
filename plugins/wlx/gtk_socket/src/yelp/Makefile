CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0` -lyelp

all:
		$(CC) $(CFLAGS) kostyl.c $(INCLUDES) -o ../../scripts/$(shell basename $(realpath .))/kostyl

clean:
		$(RM) ../../scripts/$(shell basename $(realpath .))/kostyl
