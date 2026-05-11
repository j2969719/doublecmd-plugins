#!/usr/bin/env python3

import os
import sys
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('EvinceDocument', '3.0')
gi.require_version('EvinceView', '3.0')
from gi.repository import Gtk, GLib
from gi.repository import EvinceDocument
from gi.repository import EvinceView

wid = int(sys.argv[1])
uri = GLib.filename_to_uri(sys.argv[2])
plug = Gtk.Plug()
plug.construct(wid)
scroll = Gtk.ScrolledWindow()
EvinceDocument.init()
doc = EvinceDocument.Document.factory_get_document(uri)
view = EvinceView.View()
model = EvinceView.DocumentModel()
model.set_document(doc)
view.set_model(model)
scroll.add(view)
plug.add(scroll)

plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
