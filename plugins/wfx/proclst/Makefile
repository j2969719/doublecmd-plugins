CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -I../../sdk
PLUGNAME = proclst.wfx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
