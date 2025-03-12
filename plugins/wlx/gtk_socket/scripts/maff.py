#!/usr/bin/env python3

import os
import sys
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import zipfile

import gi
gi.require_version('Gtk', '3.0')
try:
    gi.require_version('WebKit2', '4.0')
except ValueError:
    gi.require_version('WebKit2', '4.1')
from gi.repository import Gtk, GLib
from gi.repository import WebKit2

wid = int(sys.argv[1])
path = sys.argv[2]

l = len(path)
tmp = path[l - 5:l]
e = tmp.lower()
if e == ".maff":
    z = zipfile.ZipFile(path, "r")
    for finfo in z.infolist():
        f = finfo.filename
        n = f.find("/", 0)
        if n == -1:
            z.close()
            sys.exit(1)
        tmp = f[0:n]
        break
else:
    sys.exit(1)

z.extractall("/tmp/_dc~~~")
z.close()

plug = Gtk.Plug()
plug.construct(wid)
view = WebKit2.WebView()
plug.add(view)

settings = view.get_settings()
settings.set_allow_file_access_from_file_urls(True)
settings.set_allow_universal_access_from_file_urls(True)
view.set_settings(settings)

uri = GLib.filename_to_uri("/tmp/_dc~~~/" + tmp + "/index.html")
view.load_uri(uri)

plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
