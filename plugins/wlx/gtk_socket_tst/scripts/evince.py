#!/usr/bin/env python3

import os
import sys
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('EvinceDocument', '3.0')
gi.require_version('EvinceView', '3.0')
gi.require_version('Gio', '2.0')
from gi.repository import Gio
from gi.repository import Gtk, GLib
from gi.repository import EvinceDocument
from gi.repository import EvinceView

widgets = {}
socket_path = os.environ['SOCKET']

def create_ui(xid):
	plug = Gtk.Plug()
	plug.construct(xid)
	scroll = Gtk.ScrolledWindow()
	view = EvinceView.View()
	scroll.add(view)
	plug.add(scroll)
	plug.show_all()
	widgets[xid] = {"plug": plug, "view": view}
	return True

def loadfile(xid, filename):
	uri = GLib.filename_to_uri(filename)
	doc = EvinceDocument.Document.factory_get_document(uri)
	model = EvinceView.DocumentModel()
	model.set_document(doc)
	widgets[xid]["doc"] = doc
	widgets[xid]["view"].set_model(model)
	return True

def destroy(xid):
	data = widgets.pop(xid) 
	data["plug"].destroy()
	return True

def trigger(xid, command):
	view = widgets[xid]["view"]
	if command == "copy":
		view.copy()
	elif command == "select_all":
		view.select_all()
	view.find_set_highlight_search(False)
	return True

def textsearch(xid, text, flags):
	lcs_findfirst  = 1
	lcs_matchcase  = 2
	lcs_wholewords = 4
	lcs_backwards  = 8

	view = widgets[xid]["view"]
	view.find_set_highlight_search(True)

	if flags & lcs_findfirst:
		pages = widgets[xid]["doc"].get_n_pages()
		job = EvinceView.JobFind.new(widgets[xid]["doc"], 0, pages, text, flags & lcs_matchcase)
		view.find_started(job);
		EvinceView.Job.scheduler_push_job(job, EvinceView.JobPriority.PRIORITY_HIGH)
	elif flags & lcs_backwards:
		view.find_previous()
	else:
		view.find_next()
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

EvinceDocument.init()
Gtk.main()
