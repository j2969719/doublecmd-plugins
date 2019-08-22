CC=g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
LIBS= `pkg-config --cflags --libs Qt5WebKit Qt5WebKitWidgets` -I../../sdk

all:
		$(CC) $(CFLAGS) $(LIBS) wlxwebkit.cpp -o hx_webkit_qt_crap.wlx

clean:
		$(RM) hx_webkit_qt_crap.wlx
