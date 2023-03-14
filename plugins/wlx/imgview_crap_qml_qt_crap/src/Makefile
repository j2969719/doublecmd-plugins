CC=g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
LIBS= `pkg-config --cflags --libs Qt5Widgets Qt5QuickWidgets` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))
DETECT_STRING =

all:
		$(CC) $(CFLAGS) $(LIBS) plugin.cpp -o ../$(PLUGNAME) -D'DETECT_STRING="$(DETECT_STRING)"'

clean:
		$(RM) ../$(PLUGNAME)
