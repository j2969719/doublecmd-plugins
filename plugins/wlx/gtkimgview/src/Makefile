CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtkimageview` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) gtkimgview.c -o ../$(PLUGNAME) -D'PLUGNAME="$(PLUGNAME)"'

clean:
		$(RM) ../$(PLUGNAME)
