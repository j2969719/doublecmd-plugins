CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs abiword-3.0`


all:
		$(CC) $(CFLAGS) kostyl.c $(INCLUDES) -o ../../scripts/$(shell basename $(realpath .))/kostyl

clean:
		$(RM) ../../scripts/$(shell basename $(realpath .))/kostyl
