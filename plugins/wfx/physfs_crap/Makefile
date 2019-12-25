CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = `pkg-config --cflags --libs physfs` -I../../sdk
PLUGNAME = physfs_crap.wfx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
