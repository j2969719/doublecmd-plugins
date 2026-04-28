#!/usr/bin/env python3

import os
import sys
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')
try:
    gi.require_version('WebKit2', '4.0')
except ValueError:
    gi.require_version('WebKit2', '4.1')
from gi.repository import Gtk, GLib
from gi.repository import WebKit2

wid = int(sys.argv[1])
uri = GLib.filename_to_uri(sys.argv[2])
plug = Gtk.Plug()
plug.construct(wid)
view = WebKit2.WebView()
plug.add(view)

settings = view.get_settings()
settings.set_allow_file_access_from_file_urls(True)
settings.set_allow_universal_access_from_file_urls(True)
view.set_settings(settings)

view.load_uri(uri)

plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
