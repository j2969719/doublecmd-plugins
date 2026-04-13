#!/usr/bin/env python3

import os
import sys
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Gio', '2.0')
gi.require_version('Abi', '3.0')
from gi.repository import Gio
from gi.repository import Gtk, GLib
from gi.repository import Abi

widgets = {}
socket_path = os.environ['SOCKET']

def create_ui(xid):
	plug = Gtk.Plug()
	plug.construct(xid)
	view = Abi.Widget()
	#print(dir(view))
	view.view_print_layout()
	plug.add(view)
	plug.show_all()
	widgets[xid] = {"plug": plug, "view": view}
	return True

def destroy(xid):
	data = widgets.pop(xid) 
	data["plug"].destroy()
	return True

def loadfile(xid, filename):
	uri = GLib.filename_to_uri(filename)
	widgets[xid]["view"].load_file(filename, "")
	return True

def textsearch(xid, text, flags):
	lcs_findfirst  = 1
	lcs_matchcase  = 2
	lcs_wholewords = 4
	lcs_backwards  = 8

	view = widgets[xid]["view"]
	view.set_find_string(text)
	if flags & lcs_backwards:
		view.find_prev()
	else:
		view.find_next(flags & lcs_findfirst)
	return True

def trigger(xid, command):
	view = widgets[xid]["view"]
	if command == "copy":
		view.copy()
	elif command == "select_all":
		view.select_all()
	return True

def on_read_ready(stream, result, user_data):
	try:
		line, length = stream.read_line_finish_utf8(result)

		if not line:
			print(f"{os.path.basename(__file__)}: line is None")
			Gtk.main_quit()
			return

		parts = line.strip().split('\t')
		command = parts[0]
		xid = int(parts[1])
		param = parts[2]
		flags = int(parts[3])
		is_ok = False

		match command:
			case "?CREATE":
				is_ok = create_ui(xid)
			case "?LOAD":
				is_ok = loadfile(xid, param)
			case "?DESTROY":
				is_ok = destroy(xid)
			case "?COPY":
				is_ok = trigger(xid, "copy")
			case "?SELECTALL":
				is_ok = trigger(xid, "select_all")
			case "?FIND":
				is_ok = textsearch(xid, param, flags)

		print(f"{os.path.basename(__file__)}: {command} -> {is_ok}")

		if is_ok:
			data_out.put_string("!OK\n", None)
		else:
			data_out.put_string("!ERROR\n", None)
		out_stream.flush(None)
		data_in.read_line_async(GLib.PRIORITY_DEFAULT, None, on_read_ready, None)

	except Exception as e:
		print(f"{os.path.basename(__file__)}: {e}")
		Gtk.main_quit()
		return

address = Gio.UnixSocketAddress.new(socket_path)
client = Gio.SocketClient.new()
conn = client.connect(address, None)
out_stream = conn.get_output_stream()
in_stream = conn.get_input_stream()
data_in = Gio.DataInputStream.new(in_stream)
data_out = Gio.DataOutputStream.new(out_stream)
data_out.put_string("READY\n", None)
out_stream.flush(None)
data_in.read_line_async(GLib.PRIORITY_DEFAULT, None, on_read_ready, None)

Abi.init_noargs() 
Gtk.main()
