CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --libs --cflags gtk+-2.0 poppler-glib` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) wlxPView.c -o ../wlxPView.wlx

clean:
		$(RM) ../wlxPView.wlx
