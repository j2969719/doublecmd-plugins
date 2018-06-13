CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) filelistwfx.c -o filelist.wfx

clean:
		$(RM) filelist.wfx
