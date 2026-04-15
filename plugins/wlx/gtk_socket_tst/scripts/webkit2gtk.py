#!/usr/bin/env python3

import os
import sys
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('WebKit2', '4.0')
from gi.repository import Gtk, GLib
from gi.repository import WebKit2
import subprocess
import shutil

widgets = {}
mutool_exts = ['.epub', '.fb2']
hx_exts = ['.odt', '.ods', '.docx', '.doc', '.rtf', '.xls', '.xlsx', ".ppt", ".pptx", ".123", ".mdb", ".mpp", ".eml", ".dxf", ".drw", ".pct", ".wpd", ".vsd", ".shw", ".sdw", ".qpw", ".prz", ".pst"] # ".mht"

script_name = os.path.basename(__file__)
scream = "is missing, make sure nothing is registered for it in settings.ini"
try:
	import markdown
except:
	print(f"{script_name}: markdown {scream}", file=sys.stderr, flush=True)
hx_exe = os.path.join(os.path.dirname(__file__), "redist/exsimple")
hx_cfg = os.path.join(os.path.dirname(__file__), "redist/default.cfg")
is_hx_there = os.access(hx_exe, os.X_OK)
is_mupdf_there = bool(shutil.which("mutool"))
is_md_there = 'markdown' in sys.modules
if is_hx_there:
	import tempfile
else:
	print(f"{script_name}: /redist/exsimple {scream}", file=sys.stderr, flush=True)
if not is_mupdf_there:
	print(f"{script_name}: mutool {scream}", file=sys.stderr, flush=True)

def convert_using_hx(path, output):
	try:
		if os.path.exists(output):
			os.remove(output)
		cmd = [hx_exe, path, output, hx_cfg]
		subprocess.run(cmd, check=True, capture_output=True)
		if os.path.exists(output) and os.path.getsize(output) > 0:
			return True
	except:
		print(f"{script_name}: convert_using_hx fail", file=sys.stderr, flush=True)
	return False

def convert_using_mutool(path):
	try:
		cmd = ["mutool", "convert", "-F", "xhtml", "-O", "preserve-images","-o", "-", path]
		result = subprocess.run(cmd, check=True, capture_output=True, text=True)
		return result.stdout
	except:
		print(f"{script_name}: convert_using_mutool fail", file=sys.stderr, flush=True)
	return None

def md_to_html(path):
	try:
		with open(path, 'r') as file:
			return markdown.markdown(file.read(), extensions=['extra'])
	except:
		print(f"{script_name}: md_to_html fail", file=sys.stderr, flush=True)
	return None

def on_decide_policy(view, decision, decision_type, *args):
	if decision_type != WebKit2.PolicyDecisionType.NAVIGATION_ACTION:
		return False
	action = decision.get_navigation_action()
	uri = action.get_request().get_uri()
	try:
		if "://" not in uri:
			dirname = getattr(view, "dirname", None)
			path = os.path.join(dirname, uri)
		else:
			path, _ = GLib.filename_from_uri(uri)
		ext = os.path.splitext(path)[1].lower()
		if is_md_there and ext == ".md":
			html = md_to_html(path)
		elif is_mupdf_there and ext in mutool_exts:
			html = convert_using_mutool(path)
		elif is_hx_there and ext in hx_exts:
			tmpdir = getattr(view, "tmpdir", None)
			output = os.path.join(tmpdir, "output.html")
			if convert_using_hx(path, output):
				uri = GLib.filename_to_uri(output)
				view.load_uri(uri)
				decision.ignore()
				return True
		if html:
			view.load_html(html, None)
			decision.ignore()
			return True
	except:
		pass
	return False

def create_ui(xid):
	plug = Gtk.Plug()
	plug.construct(xid)
	view = WebKit2.WebView()
	view.connect("decide-policy", on_decide_policy)
	settings = view.get_settings()
	settings.set_allow_file_access_from_file_urls(True)
	settings.set_allow_universal_access_from_file_urls(True)
	view.set_settings(settings)
	if is_hx_there:
		view.tmpdir = tempfile.mkdtemp()
	plug.add(view)
	plug.show_all()
	widgets[xid] = {"plug": plug, "view": view}
	return True

def destroy(xid):
	data = widgets.pop(xid) 
	if is_hx_there:
		tmpdir = getattr(data["view"], "tmpdir", None)
		shutil.rmtree(tmpdir, ignore_errors=True)
	data["plug"].destroy()
	return True

def load_file(xid, filename):
	ext = os.path.splitext(filename)[1].lower()
	if not is_md_there and ext == ".md":
		return False
	elif not is_mupdf_there and ext in mutool_exts:
		return False
	elif not is_hx_there and ext in hx_exts:
		return False
	view = widgets[xid]["view"]
	uri = GLib.filename_to_uri(filename)
	view.dirname = os.path.dirname(filename)
	view.load_uri(uri)
	return True

def search_text(xid, text, flags):
	lcs_matchcase  = 2
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
		print(f"{script_name}: {e}", file=sys.stderr, flush=True)
		Gtk.main_quit()
		return False
	return True

channel = GLib.IOChannel.unix_new(sys.stdin.fileno())
GLib.io_add_watch(channel, GLib.PRIORITY_DEFAULT, GLib.IO_IN | GLib.IO_HUP, on_read_ready)
sys.stdout.write("READY\n")
sys.stdout.flush()

Gtk.main()
