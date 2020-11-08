CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -I../../sdk
PLUGNAME = skipdotfiles.wdx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)

