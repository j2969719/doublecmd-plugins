#!/usr/bin/env python3

import os
import sys
import gi
gi.require_version('Gtk', '3.0')
try:
	gi.require_version('WebKit2', '4.1')
except:
	gi.require_version('WebKit2', '4.0')
from gi.repository import Gtk, GLib, Gio
from gi.repository import WebKit2
import subprocess
import tempfile
import shutil
import glob
import html
import re

widgets = {}
mutool_exts = ['.epub', '.fb2']
hx_exts = ['.odt', '.ods', '.docx', '.doc', '.rtf', '.xls', '.xlsx', ".ppt", ".pptx", ".123", ".mdb", ".mpp", ".eml", ".dxf", ".drw", ".pct", ".wpd", ".vsd", ".shw", ".sdw", ".qpw", ".prz", ".pst"]

script_name = os.path.basename(__file__)
try:
	import markdown
except:
	pass
try:
	from lxml import etree
except:
	pass
try:
	import chardet
except:
	pass
try:
	from libzim.reader import Archive
except:
	pass
hx_exe = os.path.join(os.path.dirname(__file__), "third_party/hx_redist/exsimple")
hx_cfg = os.path.join(os.path.dirname(__file__), "third_party/hx_redist/default.cfg")
fb2_xsl = os.path.join(os.path.dirname(__file__), "third_party/FB2_22_xhtml.xsl")
tmpdir_prefix = f"{os.path.splitext(os.path.basename(__file__))[0]}_"
is_hx_there = os.access(hx_exe, os.X_OK)
is_mupdf_there = bool(shutil.which("mutool"))
is_7z_there = bool(shutil.which("7z"))
is_md_there = 'markdown' in sys.modules
is_fb2_there = 'lxml.etree' in sys.modules and os.access(fb2_xsl, os.R_OK)
is_zim_there = 'Archive' in globals()
is_use_open_epub = False
is_use_hx_mht = False
is_debug = "SCRIPT_DEBUG" in os.environ
if is_hx_there:
	hx_env = os.environ.copy()
	hx_env["OIT_DATA_PATH"] = ""

def clean_tmpdir(tmpdir):
	for item in os.listdir(tmpdir):
		path = os.path.join(tmpdir, item)
		try:
			if os.path.isdir(path):
				shutil.rmtree(path)
			else:
				os.unlink(path)
		except Exception as e:
			if is_debug:
				print(f"{script_name}: clean_tmpdir {path}: {e}", file=sys.stderr, flush=True)

def fb2_to_html(path, xid):
	try:
		fb2_dom = etree.parse(path)
		xsl_dom = etree.parse(fb2_xsl)
		transform = etree.XSLT(xsl_dom)
		xhtml = transform(fb2_dom)
		if xhtml:
			store = widgets[xid]["tree_model"]
			store.clear()
			i = 0
			for link in xhtml.xpath('//a[starts-with(@href, "#TOC_id")]'):
				section = link.get("href")[1:]
				if xhtml.xpath(f'//*[@name="{section}"]'):
					i += 1
					title = link.text
					if not title:
						title = "anon section"
					uri = f"about:blank#{section}"
					store.append(None, [title, uri])
			if i > 0:
				widgets[xid]["left_side"].show()
			return str(xhtml)
	except Exception as e:
		if is_debug:
			print(f"{script_name}: fb2_to_html {path}: {e}", file=sys.stderr, flush=True)
	return None

def extact_archive(path, tmpdir):
	try:
		clean_tmpdir(tmpdir)
		cmd = ["7z", "x", path, "-o" + tmpdir, "-y"]
		result = subprocess.run(cmd, check=True, capture_output=True)
		return True
	except Exception as e:
		if is_debug:
			print(f"{script_name}: extact_archive {path}: {e}", file=sys.stderr, flush=True)
	return False

def load_with_chardet(path):
	with open(path, 'rb') as f:
		raw = f.read()
		if 'chardet' in sys.modules:
			enc = chardet.detect(raw)['encoding'] or 'utf-8'
			return raw.decode(enc, errors='replace')
		else:
			return raw_data.decode('utf-8', errors='replace')

def build_fileuri(tmpdir, subdir, src):
	if not src:
		return None
	anchor = None
	uri = GLib.filename_to_uri(os.path.join(tmpdir, subdir, src.split('#')[0]))
	try:
		anchor = src.split('#')[1]
	except:
		pass
	if anchor:
		uri = uri + '#' + anchor
	return uri

def get_file_and_subdir(tmpdir, pattern):
	files = glob.glob(os.path.join(tmpdir, pattern), recursive=True)
	if not files:
		return None, None
	subdir = os.path.dirname(files[0])
	if subdir != tmpdir:
		subdir = os.path.basename(subdir)
	else:
		subdir = ""
	return files[0], subdir

def parse_nav_xhtml(tmpdir, store, epub_ns):
	nav_file, subdir = get_file_and_subdir(tmpdir, "**/nav.?htm?")
	if not nav_file:
		return None
	first_uri = None
	nav_dom = etree.parse(nav_file)
	for item in nav_dom.xpath('//xhtml:nav//xhtml:a', namespaces=epub_ns):
		title = item.text.strip()
		src = item.get('href')
		uri = build_fileuri(tmpdir, subdir, src)
		if uri and not first_uri:
			first_uri = uri
		if uri:
			store.append(None, [title, uri])
	return first_uri

def parse_ncx(tmpdir, store, epub_ns):
	ncx_file, subdir = get_file_and_subdir(tmpdir, "**/*.ncx")
	if not ncx_file:
		return None
	first_uri = None
	ncx_dom = etree.parse(ncx_file)
	for nav_point in ncx_dom.xpath('//ncx:navPoint', namespaces=epub_ns):
		title = nav_point.xpath('./ncx:navLabel/ncx:text/text()', namespaces=epub_ns)[0]
		src = nav_point.xpath('./ncx:content/@src', namespaces=epub_ns)[0]
		uri = build_fileuri(tmpdir, subdir, src)
		if uri and not first_uri:
			first_uri = uri
		store.append(None, [title, uri])
	return first_uri

def parse_rdf(tmpdir, rdf_ns):
	rdf_file, subdir = get_file_and_subdir(tmpdir, "**/index.rdf")
	if not rdf_file:
		return None
	rdf_dom = etree.parse(rdf_file)
	src = rdf_dom.xpath('//maf:indexfilename/@rdf:resource', namespaces=rdf_ns)[0]
	return build_fileuri(tmpdir, subdir, src)

def parse_hhc(tmpdir, store):
	hhc_files = glob.glob(os.path.join(tmpdir, "*.hhc"))
	if not hhc_files:
		return None
	text = load_with_chardet(hhc_files[0])
	stack = [None]
	last_iter = None
	first_uri = None
	tags = re.split(r'(<UL>|</UL>|<OBJECT[^>]*>.*?</OBJECT>)', text, flags=re.S | re.I)
	re_name = re.compile(r'NAME\s*=\s*"Name"[ \t\r\n]+VALUE\s*=\s*"([^"]*)"', re.I)
	re_local = re.compile(r'NAME\s*=\s*"Local"[ \t\r\n]+VALUE\s*=\s*"([^"]*)"', re.I)
	for tag in tags:
		tag_upper = tag.strip().upper()
		if not tag_upper:
			continue
		if tag_upper == '<UL>':
			stack.append(last_iter)
		elif tag_upper == '</UL>':
			if len(stack) > 1:
				stack.pop()
		elif '<OBJECT' in tag_upper:
			name = re_name.search(tag)
			local = re_local.search(tag)
			if name or local:
				title = html.unescape(name.group(1)) if name else local.group(1)
				uri = None
				if local:
					uri = GLib.filename_to_uri(os.path.join(tmpdir, local.group(1).replace('\\', '/')))
					if uri and not first_uri:
						first_uri = uri
				last_iter = store.append(stack[-1], [title, uri])
	return first_uri

def open_chm(path, tmpdir, xid):
	try:
		if extact_archive(path, tmpdir):
			store = widgets[xid]["tree_model"]
			store.clear()
			uri = parse_hhc(tmpdir, store)
			if uri:
				widgets[xid]["left_side"].show()
				return uri
	except Exception as e:
		print(f"{script_name}: open_chm {path}: {e}", file=sys.stderr, flush=True)
	return None

def open_epub(path, tmpdir, xid):
	epub_ns = {
		'ncx': 'http://www.daisy.org/z3986/2005/ncx/',
		'opf': 'http://www.idpf.org/2007/opf',
		'xhtml': 'http://www.w3.org/1999/xhtml',
	}
	try:
		if extact_archive(path, tmpdir):
			store = widgets[xid]["tree_model"]
			store.clear()
			uri = parse_ncx(tmpdir, store, epub_ns)
			if not uri:
				uri = parse_nav_xhtml(tmpdir, store, epub_ns)
			if uri:
				widgets[xid]["left_side"].show()
				return uri
	except Exception as e:
		print(f"{script_name}: open_epub {path}: {e}", file=sys.stderr, flush=True)
	return None

def open_maff(path, tmpdir):
	rdf_ns = {
		'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
		'maf': 'http://maf.mozdev.org/metadata/rdf#',
	}
	try:
		if extact_archive(path, tmpdir):
			index_files = glob.glob(os.path.join(tmpdir, "*/index.*htm*"))
			if not index_files:
				return parse_rdf(tmpdir, rdf_ns)
			else:
				return GLib.filename_to_uri(index_files[0])
	except Exception as e:
		print(f"{script_name}: open_maff {path}: {e}", file=sys.stderr, flush=True)
	return None

def open_fbz(path, tmpdir, xid):
	try:
		if extact_archive(path, tmpdir):
			files = glob.glob(os.path.join(tmpdir, "*.fb2"))
			if files:
				return fb2_to_html(files[0], xid)
	except Exception as e:
		print(f"{script_name}: open_fbz {path}: {e}", file=sys.stderr, flush=True)
	return None

def convert_using_hx(path, tmpdir):
	try:
		clean_tmpdir(tmpdir)
		output = os.path.join(tmpdir, "output.html")
		cmd = [hx_exe, path, output, hx_cfg]
		subprocess.run(cmd, check=True, capture_output=True, env=hx_env)
		if os.path.exists(output) and os.path.getsize(output) > 0:
			return GLib.filename_to_uri(output)
	except Exception as e:
		if is_debug:
			print(f"{script_name}: convert_using_hx {path}: {e}", file=sys.stderr, flush=True)
	return False

def convert_using_mutool(path):
	try:
		cmd = ["mutool", "convert", "-F", "xhtml", "-O", "preserve-images","-o", "-", path]
		result = subprocess.run(cmd, check=True, capture_output=True, text=True)
		return result.stdout
	except Exception as e:
		if is_debug:
			print(f"{script_name}: convert_using_mutool {path}: {e}", file=sys.stderr, flush=True)
	return None

def md_to_html(path):
	try:
		with open(path, 'r') as file:
			return markdown.markdown(file.read(), extensions=['extra'])
	except Exception as e:
		if is_debug:
			print(f"{script_name}: md_to_html {path}: {e}", file=sys.stderr, flush=True)
	return None

def on_tree_cursor_changed(treeview, view):
	selection = treeview.get_selection()
	model, tree_iter = selection.get_selected()
	if tree_iter:
		uri = model.get_value(tree_iter, 1)
		if uri:
			view.load_uri(uri)

def on_decide_policy(view, decision, decision_type, *args):
	if decision_type != WebKit2.PolicyDecisionType.NAVIGATION_ACTION:
		return False
	action = decision.get_navigation_action()
	uri = action.get_request().get_uri()
	if uri.startswith("zim://data/"):
		return False
	html = None
	try:
		if "://" not in uri:
			dirname = getattr(view, "dirname", None)
			path = os.path.join(dirname, uri)
			if os.path.isdir(path):
				view.load_uri(GLib.filename_to_uri(path))
				decision.ignore()
				return True
		else:
			path, _ = GLib.filename_from_uri(uri)
		ext = os.path.splitext(path)[1].lower()
		if is_md_there and ext == ".md":
			html = md_to_html(path)

		if html:
			view.load_html(html, None)
			decision.ignore()
			return True
	except Exception as e:
			if is_debug:
				print(f"{script_name}: on_decide_policy: {e}", file=sys.stderr, flush=True)
	return False

def on_zim_uri_request(request):
	view = request.get_web_view()
	archive = getattr(view, "zim_archive", None)
	if not archive:
		return
	uri = request.get_uri()
	try:
		file_uri = uri.replace("zim://data/", "file:///")
		path, _ = GLib.filename_from_uri(file_uri)
		entry = archive.get_entry_by_path(path[1:])
		item = entry.get_item()
		data = item.content.tobytes()
		g_input_stream = Gio.MemoryInputStream.new_from_data(data, None)
		request.finish(g_input_stream, len(data), item.mimetype)
	except Exception as e:
		if is_debug:
			print(f"{script_name}: on_zim_uri_request {uri}: {e}", file=sys.stderr, flush=True)

def create_ui(xid):
	plug = Gtk.Plug()
	plug.construct(xid)
	paned = Gtk.Paned(orientation=Gtk.Orientation.HORIZONTAL)
	store = Gtk.TreeStore(str, str)
	tree_view = Gtk.TreeView(model=store)
	column = Gtk.TreeViewColumn(None, Gtk.CellRendererText(), text=0)
	tree_view.append_column(column)
	scroll = Gtk.ScrolledWindow(width_request=200)
	scroll.add(tree_view)
	paned.pack1(scroll, False, False)
	view = WebKit2.WebView()
	paned.pack2(view, True, False)
	view.connect("decide-policy", on_decide_policy)
	tree_view.connect("cursor-changed", on_tree_cursor_changed, view)
	settings = view.get_settings()
	settings.set_allow_file_access_from_file_urls(True)
	settings.set_allow_universal_access_from_file_urls(True)
	view.set_settings(settings)
	view.tmpdir = tempfile.mkdtemp(prefix=tmpdir_prefix)
	plug.add(paned)
	plug.show_all()
	widgets[xid] = {"plug": plug, "view": view, "tree_model": store, "left_side": scroll, "column": column}
	return True

def destroy(xid):
	data = widgets.pop(xid) 
	tmpdir = getattr(data["view"], "tmpdir", None)
	shutil.rmtree(tmpdir, ignore_errors=True)
	data["plug"].destroy()
	return True

def load_file(xid, filename):
	ext = os.path.splitext(filename)[1].lower()
	is_fbz = filename.lower().endswith(".fb2.zip") or ext == ".fbz"
	if not is_md_there and ext == ".md":
		return False
	if not is_7z_there and (ext == ".chm" or ext == ".maff"):
		return False
	if (not is_7z_there or not is_fb2_there) and is_fbz:
		return False
	elif not is_mupdf_there and ext in mutool_exts:
		if ext ==".fb2":
			if not is_fb2_there:
				return False
		elif is_use_open_epub and ext ==".epub":
			if not is_7z_there:
				return False
		else:
			return False
	elif not is_hx_there and ext in hx_exts:
		return False
	elif not is_zim_there and ext == ".zim":
		return False
	widgets[xid]["left_side"].hide()
	view = widgets[xid]["view"]
	view.dirname = os.path.dirname(filename)
	if hasattr(view, 'zim_archive'):
		del view.zim_archive
	uri = None
	html = None
	tmpdir = getattr(view, "tmpdir", None)
	widgets[xid]["column"].set_title(os.path.basename(filename).replace("_", " "))

	if is_fb2_there and ext ==".fb2":
		html = fb2_to_html(filename, xid)
	if is_fb2_there and is_fbz:
		html = open_fbz(filename, tmpdir, xid)
	elif is_use_open_epub and ext == ".epub":
		uri = open_epub(filename, tmpdir, xid)

	if not html and not uri:
		if ext == ".chm":
			uri = open_chm(filename, tmpdir, xid)
		elif ext == ".maff":
			uri = open_maff(filename, tmpdir)
		elif ext in mutool_exts:
			html = convert_using_mutool(filename)
		elif ext in hx_exts or (is_use_hx_mht and ext == ".mht"):
			uri = convert_using_hx(filename, tmpdir)
		elif ext == ".zim":
			view.zim_archive = Archive(filename)
			if not view.zim_archive:
				return False
			main_path = view.zim_archive.main_entry.get_item().path
			if not main_path:
				return False
			uri = GLib.filename_to_uri(f"/{main_path}").replace("file:///", "zim://data/")
		else:
			uri = GLib.filename_to_uri(filename)

	if uri:
		view.load_uri(uri)
	elif html:
		view.load_html(html, None)
	else:
		return False

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
				if not is_ok:
					destroy(xid)
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

if is_zim_there:
	WebKit2.WebContext.get_default().register_uri_scheme("zim", on_zim_uri_request)

try:
	cfg = GLib.KeyFile()
	cfg.load_from_file(os.environ["PLUG_CFGFILE"], GLib.KeyFileFlags.NONE)
	is_use_open_epub = cfg.get_boolean("epub", f"{script_name}!use_open_epub")
	is_use_hx_mht = cfg.get_boolean("mht", f"{script_name}!use_hx")
except:
	pass

if is_debug:
	print(f"{script_name}: markdown support {is_md_there}", file=sys.stderr, flush=True)
	print(f"{script_name}: fb2 support {is_fb2_there}", file=sys.stderr, flush=True)
	print(f"{script_name}: outsidein html export support {is_hx_there}", file=sys.stderr, flush=True)
	print(f"{script_name}: mupdf support {is_mupdf_there}", file=sys.stderr, flush=True)
	print(f"{script_name}: 7zip support {is_7z_there}", file=sys.stderr, flush=True)
	print(f"{script_name}: zim support {is_zim_there}", file=sys.stderr, flush=True)
	print(f"{script_name}: force builtin epub handling {is_use_open_epub}", file=sys.stderr, flush=True)
	print(f"{script_name}: force outsidein html export for mht {is_use_hx_mht}", file=sys.stderr, flush=True)

channel = GLib.IOChannel.unix_new(sys.stdin.fileno())
GLib.io_add_watch(channel, GLib.PRIORITY_DEFAULT, GLib.IO_IN | GLib.IO_HUP, on_read_ready)
sys.stdout.write("READY\n")
sys.stdout.flush()

Gtk.main()
