.PHONY: all clean install

CXX = g++
CXXFLAGS = -shared -fPIC -Wl,--no-as-needed -O2 -I/home/pplupo/mdk-sdk/include

# Qt6 public + private + OpenGL
QT6_FLAGS = $(shell pkg-config --cflags --libs Qt6Widgets Qt6Core Qt6Gui Qt6OpenGL Qt6OpenGLWidgets)
QT6_PRIVATE = -I/usr/include/qt6/QtGui/6.11.0 -I/usr/include/qt6/QtGui/6.11.0/QtGui

LDFLAGS = -ldl

PLUGNAME = wlx_mdk_wayland.wlx
INSTALL_DIR = /home/pplupo/.config/doublecmd/plugins/wlx

# Find the Qt6 moc binary
MOC6 := /usr/lib/qt6/moc

all: $(PLUGNAME)

plugin.moc: plugin.cpp
	$(MOC6) -i plugin.cpp -o plugin.moc

$(PLUGNAME): plugin.moc plugin.cpp
	$(CXX) $(CXXFLAGS) -o $@ plugin.cpp $(QT6_FLAGS) $(QT6_PRIVATE) $(LDFLAGS)

install: all
	mkdir -p $(INSTALL_DIR)
	cp -f $(PLUGNAME) $(INSTALL_DIR)/

clean:
	rm -f $(PLUGNAME) plugin.moc
