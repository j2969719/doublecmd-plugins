#!/usr/bin/env python3

import os
import sys
import chardet
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango
gi.require_version('GtkSource', '4')
from gi.repository import GtkSource

lcs_findfirst =  1
lcs_matchcase =  2
lcs_backwards =  8

lcp_wraptext  =  1
lcp_ansi      =  4
lcp_ascii     =  8
lcp_variable  = 12

widgets = {}

script_name = os.path.basename(__file__)
lang_manager = GtkSource.LanguageManager()
style_manager = GtkSource.StyleSchemeManager.get_default()
is_show_numbers = True
font = '15px "Monospace"'
theme = 'classic'
tab_width = 8

def set_wrap_mode(view, showflags):
	if showflags & lcp_wraptext:
		view.set_wrap_mode(Pango.WrapMode.WORD)
	else:
		view.set_wrap_mode(Pango.WrapMode.NONE)

def create_ui(xid, showflags):
	plug = Gtk.Plug()
	plug.construct(xid)
	scroll = Gtk.ScrolledWindow()
	view = GtkSource.View()
	scroll.add(view)
	view.set_name("sourceview")
	set_wrap_mode(view, showflags)
	provider = Gtk.CssProvider()
	css = "#sourceview { font: " + font + " ; }"
	provider.load_from_data(css)
	Gtk.StyleContext.add_provider_for_screen(Gdk.Screen.get_default(), provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
	style = style_manager.get_scheme(theme)
	view.get_buffer().set_style_scheme(style)
	view.get_space_drawer().set_enable_matrix(True)
	view.set_highlight_current_line(True)
	view.set_show_line_numbers(is_show_numbers)
	view.set_editable(False)
	view.set_tab_width(tab_width)
	plug.add(scroll)
	plug.show_all()
	widgets[xid] = {"plug": plug, "view": view}
	return True

def destroy(xid):
	data = widgets.pop(xid) 
	data["plug"].destroy()
	return True

def load_file(xid, filename, showflags):
	buffer = widgets[xid]["view"].get_buffer()
	buffer.set_text("")
	lang = lang_manager.guess_language(filename)
	if lang is None:
		return False
	buffer.set_language(lang)
	with open(filename, 'rb') as f:
		raw_data = f.read()
	encoding = chardet.detect(raw_data)['encoding']
	text = raw_data.decode(encoding or 'utf-8', errors='ignore')
	buffer.set_text(text)
	return True

def search_text(xid, text, flags):
	view = widgets[xid]["view"]
	buffer = view.get_buffer()
	last_pos = buffer.get_mark("last_pos")
	text_iter = None
	if last_pos is None or (flags & lcs_findfirst):
		if flags & lcs_backwards:
			text_iter = buffer.get_end_iter()
		else:
			text_iter = buffer.get_start_iter()
	else:
		text_iter = buffer.get_iter_at_mark(last_pos)
	opt = Gtk.TextSearchFlags.TEXT_ONLY
	if not (flags & lcs_matchcase):
		opt |= Gtk.TextSearchFlags.CASE_INSENSITIVE
	if flags & lcs_backwards:
		result = text_iter.backward_search(text, opt, None)
	else:
		result = text_iter.forward_search(text, opt, None)
	if result is not None:
		mstart, mend = result
		buffer.select_range(mstart, mend)
		target_pos = None
		if (flags & lcs_backwards): 
			target_pos = mstart
		else:
			target_pos = mend
		if last_pos is None:
			last_pos = buffer.create_mark("last_pos", target_pos, False)
		else:
			buffer.move_mark(last_pos, target_pos)
		view.scroll_mark_onscreen(last_pos)
	else:
		message = f"\"{text}\" not found!"
		dialog = Gtk.MessageDialog(message_type=Gtk.MessageType.INFO, buttons=Gtk.ButtonsType.OK, text=message)
		dialog.run()
		dialog.destroy()
	return True

def send_command(xid, command, showflags):
	view = widgets[xid]["view"]
	buffer = view.get_buffer()
	if command == "copy":
		clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
		buffer.copy_clipboard(clipboard)
	elif command == "select_all":
		start_iter = buffer.get_start_iter()
		end_iter = buffer.get_end_iter()
		buffer.select_range(start_iter, end_iter)
	elif command == "showflags_changed":
		set_wrap_mode(view, showflags)
	return True

def on_read_ready(channel, condition):
	if condition & GLib.IO_HUP:
		Gtk.main_quit()
		return False
	try:
		line = sys.stdin.readline()
		if not line:
			print(f"{script_name}: line is None", file=sys.stderr, flush=True)
			Gtk.main_quit()
			return False
		parts = line.strip().split('\t')
		command = parts[0]
		xid = int(parts[1])
		param = parts[2]
		flags = int(parts[3])
		is_ok = False
		match command:
			case "?CREATE":
				is_ok = create_ui(xid, flags)
			case "?DESTROY":
				is_ok = destroy(xid)
			case "?LOAD":
				is_ok = load_file(xid, param, flags)
			case "?COPY":
				is_ok = send_command(xid, "copy", flags)
			case "?SELECTALL":
				is_ok = send_command(xid, "select_all", flags)
			case "?FIND":
				is_ok = search_text(xid, param, flags)
			case "?NEWPARAMS":
				is_ok = send_command(xid, "showflags_changed", flags)
		if is_ok:
			sys.stdout.write("!OK\n")
		else:
			sys.stdout.write("!ERROR\n")
		sys.stdout.flush()
	except Exception as e:
		print(f"{script_name}: {e}", file=sys.stderr, flush=True)
		Gtk.main_quit()
		return False
	return True

try:
	cfg = GLib.KeyFile()
	cfg.load_from_file(os.environ["PLUG_CFGFILE"], GLib.KeyFileFlags.NONE)
	string = cfg.get_string(".", f"{script_name}!font_css")
	if not (string is None):
		font = string
except:
	pass
try:
	string = cfg.get_string(".", f"{script_name}!theme")
	if not (string is None):
		theme = string
except:
	pass
try:
	tab_width = cfg.get_integer(".", f"{script_name}!tab_width")
except:
	pass
try:
	is_show_numbers = cfg.get_boolean(".", f"{script_name}!is_show_numbers")
except:
	pass


channel = GLib.IOChannel.unix_new(sys.stdin.fileno())
GLib.io_add_watch(channel, GLib.PRIORITY_DEFAULT, GLib.IO_IN | GLib.IO_HUP, on_read_ready)
sys.stdout.write("READY\n")
sys.stdout.flush()

Gtk.main()
