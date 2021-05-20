CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -std=gnu99
INCLUDES = `pkg-config --cflags --libs glib-2.0 gio-2.0` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all: default tmppanel

default:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o ../$(PLUGNAME)

tmppanel:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o ../tp_$(PLUGNAME) -D TEMP_PANEL

clean:
		$(RM) ../$(PLUGNAME)
		$(RM) ../tp_$(PLUGNAME)
