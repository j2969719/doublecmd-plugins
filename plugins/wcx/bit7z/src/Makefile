CC = g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -I../../../sdk -Ithird_party/bit7z/include
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all: default auto

default:
		$(CC) $(CFLAGS) -o ../$(PLUGNAME) plugin.cpp -L third_party/bit7z/lib/x64 -lbit7z64 `pkg-config --cflags --libs glib-2.0` $(INCLUDES) -D'PLUGNAME="$(PLUGNAME)"'

auto:
		$(CC) $(CFLAGS) -o ../auto_$(PLUGNAME) plugin.cpp -DBIT7Z_AUTO_FORMAT -L third_party/bit7z/lib/x64 -lbit7z64 $(INCLUDES) -D'PLUGNAME="auto_$(PLUGNAME)"'

clean:
		$(RM) ../$(PLUGNAME)
		$(RM) ../auto_$(PLUGNAME)