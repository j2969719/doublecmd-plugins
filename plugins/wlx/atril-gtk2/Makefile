CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtk+-2.0 atril-view-1.5.0 atril-document-1.5.0 webkit-1.0` -I../../sdk -I.

all:
		$(CC) $(CFLAGS) $(INCLUDES) wlxatrilgtk2.c -o atril-gtk2.wlx

clean:
		$(RM) atril-gtk2.wlx
