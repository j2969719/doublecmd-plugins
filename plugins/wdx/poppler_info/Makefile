CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --cflags --libs poppler-glib` -I../../sdk
PLUGNAME = poppler_info.wdx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
