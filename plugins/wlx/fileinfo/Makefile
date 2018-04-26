CC = gcc
CFLAGS = -shared -fPIC
INCLUDES = `pkg-config --libs --cflags gtk+-2.0 gtksourceview-2.0` -I../../sdk

all:
		$(CC) $(CFLAGS) $(INCLUDES) fileinfo.c -o fileinfo_fork.wlx

clean:
		$(RM) fileinfo_fork.wlx
