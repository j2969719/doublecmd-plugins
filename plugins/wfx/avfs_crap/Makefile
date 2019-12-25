CC = gcc
CFLAGS = -shared -fPIC -O -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs avfs` -I../../sdk
PLUGNAME = avfs_crap.wfx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
