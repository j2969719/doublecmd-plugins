CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs gtkimageview` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) gtkimgview.c -o gtkimgview_crap.wlx

clean:
		$(RM) gtkimgview_crap.wlx
