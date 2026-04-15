#!/usr/bin/env python3

import sys
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('WebKit2', '4.0')
from gi.repository import Gtk, GLib
from gi.repository import WebKit2

widgets = {}

def create_ui(xid):
	plug = Gtk.Plug()
	plug.construct(xid)
	view = WebKit2.WebView()
	plug.add(view)
	plug.show_all()
	widgets[xid] = {"plug": plug, "view": view}
	return True

def destroy(xid):
	data = widgets.pop(xid) 
	data["plug"].destroy()
	return True

def load_file(xid, filename):
	uri = GLib.filename_to_uri(filename)
	widgets[xid]["view"].load_uri(uri)
	return True

def search_text(xid, text, flags):
	lcs_findfirst  = 1
	lcs_matchcase  = 2
	lcs_wholewords = 4
	lcs_backwards  = 8

	find_controller = widgets[xid]["view"].get_find_controller()
	options = WebKit2.FindOptions.NONE
	if not (flags & lcs_matchcase):
		options |= WebKit2.FindOptions.CASE_INSENSITIVE
	if flags & lcs_backwards:
		options |= WebKit2.FindOptions.BACKWARDS
	find_controller.search(text, options, 69)
	return True

def send_command(xid, command):
	if command == "copy":
		widgets[xid]["view"].execute_editing_command(WebKit2.EDITING_COMMAND_COPY)
	elif command == "select_all":
		widgets[xid]["view"].execute_editing_command(WebKit2.EDITING_COMMAND_SELECT_ALL)
	return True

def on_read_ready(channel, condition):
	try:
		line = sys.stdin.readline()

		if not line:
			print(f"{os.path.basename(__file__)}: line is None", file=sys.stderr, flush=True)
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
				is_ok = create_ui(xid)
			case "?DESTROY":
				is_ok = destroy(xid)
			case "?LOAD":
				is_ok = load_file(xid, param)
			case "?COPY":
				is_ok = send_command(xid, "copy")
			case "?SELECTALL":
				is_ok = send_command(xid, "select_all")
			case "?FIND":
				is_ok = search_text(xid, param, flags)

		if is_ok:
			sys.stdout.write("!OK\n")
		else:
			sys.stdout.write("!ERROR\n")
		sys.stdout.flush()

	except Exception as e:
		print(f"{os.path.basename(__file__)}: {e}", file=sys.stderr, flush=True)
		Gtk.main_quit()
		return False

	return True

channel = GLib.IOChannel.unix_new(sys.stdin.fileno())
GLib.io_add_watch(channel, GLib.PRIORITY_DEFAULT, GLib.IO_IN | GLib.IO_HUP, on_read_ready)
sys.stdout.write("READY\n")
sys.stdout.flush()

Gtk.main()
