CC=g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
LIBS= `pkg-config --cflags --libs Qt5WebKit Qt5WebKitWidgets md4c-html` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(LIBS) plugin.cpp -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
