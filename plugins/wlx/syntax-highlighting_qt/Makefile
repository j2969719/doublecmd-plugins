CC=g++
CFLAGS = -shared -fPIC -Wl,--no-as-needed
LIBS= `pkg-config --cflags --libs Qt5Core Qt5Widgets` -I../../sdk -I/usr/include/KF5/KSyntaxHighlighting/ -lKF5SyntaxHighlighting

all:
		$(CC) $(CFLAGS) $(LIBS) plugin.cpp -o syntax-highlighting_qt.wlx

clean:
		$(RM) syntax-highlighting_qt.wlx
