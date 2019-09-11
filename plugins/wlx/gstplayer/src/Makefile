CC = gcc
CFLAGS = -I../../../sdk -Wall -fPIC -O2

all shared static:
	$(CC) -shared -o ../gstplayer.wlx gstplayer.c $(CFLAGS) `pkg-config --cflags --libs gstreamer-video-1.0 gtk+-2.0 gstreamer-1.0`

clean:
	rm -f ../gstplayer.wlx
