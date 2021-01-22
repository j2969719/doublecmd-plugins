CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtk+-2.0 atril-view-1.5.0 atril-document-1.5.0` -I../../../sdk -I.
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) wlxatrilgtk2.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
