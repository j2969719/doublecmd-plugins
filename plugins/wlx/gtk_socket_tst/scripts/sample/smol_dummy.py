#!/usr/bin/env python3

import sys
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib

widgets = {}

def create_ui(xid):
	plug = Gtk.Plug()
	plug.construct(xid)
	#view = ...
	#plug.add(view)
	plug.show_all()
	widgets[xid] = {"plug": plug, "view": view}
	return True

def destroy(xid):
	data = widgets.pop(xid) 
	data["plug"].destroy()
	return True

def load_file(xid, filename):
	#widgets[xid]["view"].load_file(filename)
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
		is_ok = False

		match command:
			case "?CREATE":
				is_ok = create_ui(xid)
			case "?DESTROY":
				is_ok = destroy(xid)
			case "?LOAD":
				is_ok = load_file(xid, param)

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
