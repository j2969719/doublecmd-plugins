CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -lphysfs -I../../sdk
PLUGNAME = physfs_crap.wfx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
