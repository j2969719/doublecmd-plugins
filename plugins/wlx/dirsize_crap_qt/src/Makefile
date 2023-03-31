CC=g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
LIBS= `pkg-config --cflags --libs Qt5Widgets Qt5Charts Qt5Concurrent` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) plugin.cpp -o ../$(PLUGNAME) $(LIBS)

clean:
		$(RM) ../$(PLUGNAME)
