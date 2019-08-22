CC=g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
LIBS= `pkg-config --cflags --libs Qt5Core Qt5Widgets` -I../../sdk

all:
		$(CC) $(CFLAGS) $(LIBS) plugin.cpp -o fileinfo_qt.wlx

clean:
		$(RM) fileinfo_qt.wlx
