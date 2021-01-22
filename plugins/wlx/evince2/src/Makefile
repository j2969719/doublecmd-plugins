CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtk+-2.0 evince2-view-2.32 evince2-document-2.32` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) wlxevince.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
