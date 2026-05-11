#!/usr/bin/env python3

import os
import sys
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')

try:
	gi.require_version('GtkSource', '4')
except:
	gi.require_version('GtkSource', '3.0')

from gi.repository import Gtk, Pango, Gdk
from gi.repository import GtkSource

import locale
locale.setlocale(locale.LC_ALL, "")
ENCODING = locale.getpreferredencoding() or "utf_8"

wid = int(sys.argv[1])
path = sys.argv[2]

class XembedPlugin(Gtk.Plug):

	def __init__(self):
		Gtk.Plug.__init__(self)
		Gtk.Plug.construct(self, wid)

		lang_manager = GtkSource.LanguageManager()
		lang = lang_manager.guess_language(path)
		style_manager = GtkSource.StyleSchemeManager.get_default()
		style = style_manager.get_scheme("classic")

		f = open(path, "r", encoding=ENCODING, errors="replace")
		txt = f.read()
		f.close()

		self.srcbuf = GtkSource.Buffer()
		self.srcbuf.set_style_scheme(style)
		self.srcbuf.set_language(lang)
		self.srcbuf.set_text(txt)

		scroll = Gtk.ScrolledWindow()
		view = GtkSource.View.new_with_buffer(self.srcbuf)

		view.set_show_line_numbers(True)
		view.set_highlight_current_line(True)
		view.get_space_drawer().set_enable_matrix(True)
		view.set_editable(False)
		view.set_tab_width(8)
		#view.override_font(Pango.FontDescription('mono 12'))
		view.set_name("sourceview")

		cssprovider = Gtk.CssProvider()
		cssprovider.load_from_path(os.path.dirname(sys.argv[0]) + "/gtksourceview.css")
		Gtk.StyleContext.add_provider_for_screen(Gdk.Screen.get_default(), cssprovider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
		scroll.add(view)

		hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
		self.lbl_search = Gtk.Label()
		button = Gtk.Button.new_with_label("Set text")
		button.connect("pressed", self.search_dialog)
		self.btn_case = Gtk.ToggleButton.new_with_label("Case sensitive")
		self.btn_case.connect("pressed", self.search_case)
		self.btn_regex = Gtk.ToggleButton.new_with_label("RegEx")
		self.btn_regex.connect("pressed", self.search_regex)
		hbox.pack_start(self.btn_case, False, True, 2)
		hbox.pack_start(self.btn_regex, False, True, 2)
		hbox.pack_start(button, False, True, 2)
		hbox.pack_end(self.lbl_search, True, True, 1)
		hbox.set_border_width(5)

		win = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
		win.pack_start(scroll, True, True, 1)
		win.pack_end(hbox, False, True, 1)

		self.sopts = GtkSource.SearchSettings()
		self.scontext = GtkSource.SearchContext(buffer=self.srcbuf, settings=self.sopts)

		self.add(win)

	def search_dialog(self, widget):
		txt = self.lbl_search.get_text()
		self.dialog = Gtk.Dialog("Highlight text")
		self.dialog.add_buttons(Gtk.STOCK_CANCEL, Gtk.ResponseType.REJECT, Gtk.STOCK_OK, Gtk.ResponseType.OK)
		contr = Gtk.HBox()
		entry = Gtk.Entry()
		entry.set_text(txt)
		entry.set_activates_default(True)
		entry.set_size_request(350,-1)
		contr.pack_end(entry, True, True, 0)
		contr.set_border_width(5)
		contr.show_all()
		self.dialog.get_content_area().add(contr)
		self.dialog.set_default_response(Gtk.ResponseType.OK)
		response = self.dialog.run()
		txt = entry.get_text()
		self.dialog.destroy()

		if response == Gtk.ResponseType.OK:
			self.lbl_search.set_text(txt)
			self.sopts.set_search_text(txt)

	def search_regex(self, widget):
		self.sopts.set_regex_enabled(not self.btn_regex.get_active())

	def search_case(self, widget):
		self.sopts.set_case_sensitive(not self.btn_case.get_active())

plug = XembedPlugin()
plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
