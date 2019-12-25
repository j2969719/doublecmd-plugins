CC = gcc
CFLAGS = -shared -fPIC -O -Wl,--no-as-needed
INCLUDES = -lavfs -D_FORTIFY_SOURCE=2 -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -I../../sdk
PLUGNAME = avfs_crap.wfx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
