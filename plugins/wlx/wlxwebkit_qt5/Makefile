CC=g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
LIBS= `pkg-config --cflags --libs Qt5WebKit Qt5WebKitWidgets` -I../../sdk

all:
		$(CC) $(CFLAGS) $(LIBS) wlxwebkit.cpp -o wlxwebkit_qt5.wlx

clean:
		$(RM) wlxwebkit_qt5.wlx
