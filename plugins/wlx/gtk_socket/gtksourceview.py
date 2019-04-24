#!/usr/bin/python

import os
import sys
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('GtkSource', '4')
from gi.repository import Gtk, Pango
from gi.repository import GtkSource

import locale
locale.setlocale(locale.LC_ALL, "")
ENCODING = locale.getpreferredencoding() or "utf_8"

wid = int(sys.argv[1])
path = sys.argv[2]
f = open(path, "r", encoding=ENCODING, errors="replace")
txt = f.read()
f.close()

plug = Gtk.Plug()
plug.construct(wid)

lang_manager = GtkSource.LanguageManager()
lang = lang_manager.guess_language(path)
style_manager = GtkSource.StyleSchemeManager.get_default()
style = style_manager.get_scheme("builder")

srcbuf = GtkSource.Buffer()
srcbuf.set_style_scheme(style)
srcbuf.set_language(lang)
srcbuf.set_text(txt)

scroll = Gtk.ScrolledWindow()
view = GtkSource.View.new_with_buffer(srcbuf)

view.set_show_line_numbers(True)
view.set_highlight_current_line(True)
view.get_space_drawer().set_enable_matrix(True)
view.set_editable(False)
view.set_tab_width(8)
view.override_font(Pango.FontDescription('mono 13'))

scroll.add(view)
plug.add(scroll)

plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
