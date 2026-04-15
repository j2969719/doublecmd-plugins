#!/usr/bin/env python3

import sys
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib

# search flags:
lcs_findfirst  = 1 # Search from the beginning of the first displayed line (not set: find next)
lcs_matchcase  = 2 # The search string is to be treated case-sensitively.
lcs_wholewords = 4 # Find whole words only
lcs_backwards  = 8 # Search backwards towards the beginning of the file

# show flags:
lcp_wraptext      =  1 # Text: Word wrap mode is checked
lcp_fittowindow   =  2 # Images: Fit image to window is checked
lcp_ansi          =  4 # Ansi charset is checked
lcp_ascii         =  8 # Ascii(DOS) charset is checked
lcp_variable      = 12 # Variable width charset is checked
lcp_fitlargeronly = 32 # Images: Sent in addition to lcp_fittowindow if only images larger than the client are should be resized - smaller should be shown centered
lcp_center        = 64 # Images: Sent when the image needs to be centered

widgets = {}

def create_ui(xid, showflags):
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

def load_file(xid, filename, showflags):
	#view = widgets[xid]["view"]
	#if not view.load_file(filename):
	#	return False
	return True

def search_text(xid, text, flags):
	#view = widgets[xid]["view"]
	#view.set_find_string(text)
	#if flags & lcs_backwards:
	#	view.find_prev()
	#else:
	#	view.find_next(flags & lcs_findfirst)
	return True

def send_command(xid, command, showflags):
	#view = widgets[xid]["view"]
	#if command == "copy":
	#	view.copy()
	#elif command == "select_all":
	#	view.select_all()
	#elif command == "showflags_changed":
	#	view.wrap_text(showflags & lcp_wraptext)
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
		print(f"{os.path.basename(__file__)}: {e}", file=sys.stderr, flush=True)
		Gtk.main_quit()
		return False

	return True

channel = GLib.IOChannel.unix_new(sys.stdin.fileno())
GLib.io_add_watch(channel, GLib.PRIORITY_DEFAULT, GLib.IO_IN | GLib.IO_HUP, on_read_ready)
sys.stdout.write("READY\n")
sys.stdout.flush()

Gtk.main()
