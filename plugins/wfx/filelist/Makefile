CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --libs --cflags gtk+-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) filelist.c -o filelist.wfx

clean:
		$(RM) filelist.wfx
