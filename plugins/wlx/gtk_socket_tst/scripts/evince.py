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

widgets = {}

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

def destroy(xid):
	data = widgets.pop(xid)
	data["plug"].destroy()
	return True

def load_file(xid, filename):
	uri = GLib.filename_to_uri(filename)
	doc = EvinceDocument.Document.factory_get_document(uri)
	model = EvinceView.DocumentModel()
	model.set_document(doc)
	widgets[xid]["doc"] = doc
	widgets[xid]["view"].set_model(model)
	return True

def send_command(xid, command):
	view = widgets[xid]["view"]
	if command == "copy":
		view.copy()
	elif command == "select_all":
		view.select_all()
	view.find_set_highlight_search(False)
	return True

def search_text(xid, text, flags):
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

EvinceDocument.init()
Gtk.main()
