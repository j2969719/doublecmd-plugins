#!/usr/bin/env python3

import os
import sys
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib, GdkPixbuf
from pyaxmlparser import APK

widgets = {}

def create_ui(xid):
	plug = Gtk.Plug()
	plug.construct(xid)
	main_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
	vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=15)
	icon = Gtk.Image()
	title = Gtk.Label(xalign=0)
	hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=15)
	hbox.pack_start(icon, False, False, 0)
	hbox.pack_start(title, True, True, 0)
	vbox.pack_start(hbox, False, False, 0)
	scroll = Gtk.ScrolledWindow()
	info = Gtk.Label(xalign=0, yalign=0)
	scroll.add(info)
	vbox.pack_start(scroll, True, True, 0)
	main_box.pack_start(vbox, True, True, 10)
	plug.add(main_box)
	plug.show_all()
	widgets[xid] = {"plug": plug, "icon": icon, "title": title, "info": info}
	return True

def destroy(xid):
	data = widgets.pop(xid) 
	data["plug"].destroy()
	return True

def load_file(xid, filename):
	icon = widgets[xid]["icon"]
	icon.set_from_icon_name("android-sdk", 48)
	title = widgets[xid]["title"]
	title.set_text("")
	info = widgets[xid]["info"]
	info.set_text("")
	try:
		apk = APK(filename)
		#print(f"all crap:\n {dir(apk)}", file=sys.stderr, flush=True)
		icon_data = apk.icon_data
		if icon_data:
			loader = GdkPixbuf.PixbufLoader()
			loader.write(icon_data)
			loader.close()
			pixbuf = loader.get_pixbuf()
			icon.set_from_pixbuf(pixbuf.scale_simple(48, 48, GdkPixbuf.InterpType.BILINEAR))
		title.set_markup(f"<big><b>{apk.application}</b></big>\n{apk.package}")
		markup = f"<b>version:</b> {apk.version_name or "n/a"}"
		markup = f"{markup}\n<b>version_code:</b> {apk.version_code or "n/a"}"
		markup = f"{markup}\n<b>min_sdk_version:</b> {apk.get_min_sdk_version() or "n/a"}"
		markup = f"{markup}\n<b>target_sdk_version:</b> {apk.get_target_sdk_version() or "n/a"}"
		markup = f"{markup}\n<b>is_androidtv:</b> {apk.is_androidtv() or "n/a"}"
		markup = f"{markup}\n<b>is_signed:</b> {apk.is_signed() or "n/a"}"
		permissions = apk.get_permissions()
		if permissions:
			markup = f"{markup}\n\n<b>permissions:</b><tt>\n\t{"\n\t".join(permissions)}</tt>"
		features = apk.get_features()
		if features:
			markup = f"{markup}\n\n<b>features:</b><tt>\n\t{"\n\t".join(features)}</tt>"
		libraries = apk.get_libraries()
		if libraries:
			markup = f"{markup}\n\n<b>libraries:</b><tt>\n\t{"\n\t".join(libraries)}</tt>"
		services = apk.get_services()
		if services:
			markup = f"{markup}\n\n<b>services:</b><tt>\n\t{"\n\t".join(services)}</tt>"
		info.set_markup(markup)
	except Exception as e:
		print(f"{os.path.basename(__file__)}: {e}", file=sys.stderr, flush=True)
		return False
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
GLib.io_add_watch(channel, GLib.PRIORITY_DEFAULT, GLib.IO_IN, on_read_ready)
sys.stdout.write("READY\n")
sys.stdout.flush()

Gtk.main()
