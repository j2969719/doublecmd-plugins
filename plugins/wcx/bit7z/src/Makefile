CC = g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -I../../../sdk -Ithird_party/bit7z/include
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) -o ../$(PLUGNAME) plugin.cpp -L third_party/bit7z/lib/x64 -lbit7z64 $(INCLUDES) -D'PLUGNAME="$(PLUGNAME)"'


clean:
		$(RM) ../$(PLUGNAME)