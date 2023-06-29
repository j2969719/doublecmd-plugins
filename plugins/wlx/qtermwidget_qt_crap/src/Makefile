CC=g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
LIBS= `pkg-config --cflags --libs qtermwidget5` -I../../../sdk
PLUGNAME = $(shell basename $(realpath ..)).$(shell basename $(realpath ../..))

all:
		$(CC) $(CFLAGS) $(LIBS) plugin.cpp -o ../$(PLUGNAME)

clean:
		$(RM) ../$(PLUGNAME)
