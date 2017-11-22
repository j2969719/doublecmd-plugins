CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs MagickWand gtkimageview` -I../../sdk


all:
		$(CC) $(CFLAGS) $(INCLUDES) wlximagemagick.c -o wlximagemagick.wlx

clean:
		$(RM) wlximagemagick.wlx
