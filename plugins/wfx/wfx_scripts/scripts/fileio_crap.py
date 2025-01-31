#!/bin/env python3

import os
import sys
import json
import requests
import calendar
import mimetypes
import gi

from datetime import datetime, timezone

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

verb = sys.argv[1]
scr = os.path.basename(__file__)
conf = os.path.join(os.path.dirname(os.environ['COMMANDER_INI']), 'j2969719.json')

try:
	with open(conf) as f:
		obj = json.loads(f.read())
		if not scr in obj:
			obj[scr] = {}
			obj[scr]['files'] = {}
		f.close()
except FileNotFoundError:
	obj = json.loads('{ "' + scr + '" : { "files" : {} }')
	with open(conf, 'w') as f:
		json.dump(obj, f)
		f.close()

def save_obj():
	with open(conf, 'w') as f:
		json.dump(obj, f)
		f.close()

def size_str(size):
	for unit in ["", "K", "M", "G", "T"]:
		if abs(size) < 1024:
			return "{:.1f} {}".format(size, unit)
		size /= 1024
	return str(size)


def vfs_init():
	print('Fs_Request_Options')
	sys.exit()

def vfs_setopt(option, value):
	if option == 'WFX_SCRIPT_STR_ACT_COPY':
		url = obj[scr]['files'][os.path.basename(value)]['url']
		clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
		clipboard.set_text(url, -1)
		clipboard.store()
		sys.exit()
	elif option == 'WFX_SCRIPT_STR_ACT_KEY':
		url = obj[scr]['files'][os.path.basename(value)]['key']
		clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
		clipboard.set_text(url, -1)
		clipboard.store()
		sys.exit()
	sys.exit(1)

def vfs_list(path):
	if path == '/' and not obj[scr]['files'] is None:
		for name in obj[scr]['files'].keys():
			if not 'size' in obj[scr]['files'][name] or obj[scr]['files'][name]['size'] is None:
				size = '-'
			else:
				size = str(obj[scr]['files'][name]['size'])
			if not 'created' in obj[scr]['files'][name] or obj[scr]['files'][name]['created'] is None:
				date = '0000-00-00 00:00:00'
			else:
				date = obj[scr]['files'][name]['created']
			print('----------\t' + date +'\t' + size + '\t' + name)
		sys.exit()
	sys.exit(1)

def vfs_getfile(src, dst):
	data = obj[scr]['files'][os.path.basename(src)]
	headers = { 'User-Agent': 'Mozilla/5.0', }
	with open(dst, 'wb') as f:
		try:
			response = requests.get(data['url'], headers = headers, stream=True)
		except Exception as e:
			print(str(e), file=sys.stderr)
			sys.exit(1)
		if response.status_code == 200:
			total = response.headers.get('content-length')
			if total is None:
				f.write(response.content)
			else:
				for data in response.iter_content(chunk_size=4096):
					f.write(data)
		else:
			print(str(response.status_code) + ": " + response.reason, file=sys.stderr)
			if response.status_code == 404:
				del obj[scr]['files'][os.path.basename(src)]
				save_obj()
			f.close()
			sys.exit(1)
		f.close()
		sys.exit()
	sys.exit(1)

def vfs_exists(path):
	if os.path.basename(path) in obj[scr]['files']:
		sys.exit()
	sys.exit(1)

def vfs_putfile(src, dst):
	url = 'https://file.io/'
	info = os.stat(src)
	name = os.path.basename(dst)
	headers = { 'User-Agent': 'Mozilla/5.0', }
	try:
		response = requests.post(url, headers = headers, files={
			'file': open(src, 'rb').read(),
		})
	except Exception as e:
			print(str(e), file=sys.stderr)
			sys.exit(1)
	if response.status_code == 200:
			res = response.json()
			obj[scr]['files'][name] = res
			obj[scr]['files'][name]['path'] = src
			obj[scr]['files'][name]['url'] = res['link']
			save_obj()
			sys.exit()
	else:
		print(response.reason, file=sys.stderr)
	sys.exit(1)

def vfs_rmfile(path):
	del obj[scr]['files'][os.path.basename(path)]
	save_obj()
	sys.exit()

def vfs_properties(path):
	data = obj[scr]['files'][os.path.basename(path)]
	print('path' + '\t' + str(data['path']))
	print('url' + '\t' + str(data['url']))
	fields=['size', 'created', 'expires', 'key', 'maxDownloads', 'autoDelete', 'id']
	for field in fields:
		if field in data and not data[field] is None:
			print('WFX_SCRIPT_STR_' + field.upper() + '\t' + str(data[field]))
	print('Fs_PropsActs WFX_SCRIPT_STR_ACT_COPY\tWFX_SCRIPT_STR_ACT_KEY')
	sys.exit()

if verb == 'init':
	vfs_init()
elif verb == 'setopt':
	vfs_setopt(sys.argv[2], sys.argv[3])
elif verb == 'list':
	vfs_list(sys.argv[2])
elif verb == 'copyout':
	vfs_getfile(sys.argv[2], sys.argv[3])
elif verb == 'exists':
	vfs_exists(sys.argv[2])
elif verb == 'copyin':
	vfs_putfile(sys.argv[2], sys.argv[3])
elif verb == 'rm':
	vfs_rmfile(sys.argv[2])
elif verb == 'properties':
	vfs_properties(sys.argv[2])

sys.exit(1)
