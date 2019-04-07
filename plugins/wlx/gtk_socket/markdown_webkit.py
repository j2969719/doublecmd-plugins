#!/usr/bin/python

import os
import sys
import markdown
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('WebKit2', '4.0')
from gi.repository import Gtk, GLib
from gi.repository import WebKit2

wid = int(sys.argv[1])
path = sys.argv[2]

plug = Gtk.Plug()
plug.construct(wid)
view = WebKit2.WebView()
plug.add(view)

mdfile = open(path)
html = markdown.markdown(mdfile.read(), extensions=['extra'])
mdfile.close()

view.load_html(html)

plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
