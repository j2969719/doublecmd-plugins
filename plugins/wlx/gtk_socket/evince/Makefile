CC = gcc
CFLAGS =
INCLUDES = `pkg-config --cflags --libs evince-view-3.0 evince-document-3.0`

all:
		$(CC) $(CFLAGS) $(INCLUDES) kostyl.c -o kostyl

clean:
		$(RM) kostyl
