CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs MagickWand gtkimageview` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(INCLUDES) wlximagemagick.c -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
