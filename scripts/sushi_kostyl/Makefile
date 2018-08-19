CC = gcc
CFLAGS = -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs glib-2.0`

all:
		$(CC) $(CFLAGS) $(INCLUDES) sushi_kostyl.c -o sushi_kostyl

clean:
		$(RM) sushi_kostyl
