CC=g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
LIBS= `pkg-config --cflags --libs Qt5Core Qt5Widgets` -I../../../sdk -Ithird_party/bit7z/include
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) plugin.cpp -o ../$(PLUGNAME) -DBIT7Z_AUTO_FORMAT -L third_party/bit7z/lib/x64 -lbit7z64 $(LIBS)

clean:
		$(RM) ../$(PLUGNAME)
