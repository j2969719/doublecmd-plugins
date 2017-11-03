CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs MagickWand gtk+-2.0 gtkimageview` -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) wlximagemagick.c -o wlximagemagick.wlx

clean:
		$(RM) wlximagemagick.wlx
