CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtkimageview` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) gtkimgview.c -o gtkimgview.wlx

clean:
		$(RM) gtkimgview.wlx
