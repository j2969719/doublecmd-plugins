CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs abiword-3.0`


all:
		$(CC) $(CFLAGS) $(INCLUDES) kostyl.c -o ../../scripts/$(shell basename $(realpath .))/kostyl

clean:
		$(RM) ../../scripts/$(shell basename $(realpath .))/kostyl
