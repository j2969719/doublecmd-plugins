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
act_copy = 'WFX_SCRIPT_STR_ACT_COPY'
conf = os.path.join(os.path.dirname(os.environ['COMMANDER_INI']), 'j2969719.json')
try:
	with open(conf) as f:
		obj = json.loads(f.read())
		if not scr in obj:
			obj[scr] = {}
			obj[scr]['files'] = {}
		f.close()
except FileNotFoundError:
	obj = json.loads('{ ' + scr + ' : { "files" : {}, "api_dev_key" : null }')
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
	if not 'api_dev_key' in obj[scr] or obj[scr]['api_dev_key'] is None:
		print('WFX_SCRIPT_STR_APIKEY')
	sys.exit()

def vfs_setopt(option, value):
	if value == '':
		sys.exit(1)
	elif option == 'WFX_SCRIPT_STR_APIKEY':
		obj[scr]['api_dev_key'] = value
		save_obj()
		sys.exit()
	elif option == act_copy:
		url = obj[scr]['files'][os.path.basename(value)]['url']
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
			if not 'date' in obj[scr]['files'][name] or obj[scr]['files'][name]['date'] is None:
				date = '0000-00-00 00:00:00'
			else:
				date = obj[scr]['files'][name]['date']
			print('----------\t' + date +'\t' + size + '\t' + name)
		sys.exit()
	sys.exit(1)

def vfs_getfile(src, dst):
	data = obj[scr]['files'][os.path.basename(src)]
	raw = 'https://pastebin.com/raw/' + os.path.basename(data['url'])
	with open(dst, 'wb') as f:
		try:
			response = requests.get(raw, stream=True)
		except Exception as e:
			print(str(e))
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
			sys.exit(1)
		f.close()
		sys.exit()
	sys.exit(1)

def vfs_exists(path):
	if os.path.basename(path) in obj[scr]['files']:
		sys.exit()
	sys.exit(1)

def vfs_putfile(src, dst):
	url = 'https://pastebin.com/api/api_post.php'
	info = os.stat(src)
	mime = mimetypes.guess_type(src)
	if not mime[0] is None and not mime[0].startswith('text/'):
		sys.exit(1)
	name = os.path.basename(dst)
	try:
		response = requests.post(url, data = {
			'api_dev_key': obj[scr]['api_dev_key'], 
			'api_option' : 'paste',
			'api_paste_code': open(src, 'r').read(),
			'api_paste_name': name,
		})
	except Exception as e:
			print(str(e), file=sys.stderr)
			sys.exit(1)
	if response.status_code == 200:
			obj[scr]['files'][name] = {}
			obj[scr]['files'][name]['url'] = response.text
			obj[scr]['files'][name]['path'] = src
			obj[scr]['files'][name]['size'] = str(info.st_size)
			obj[scr]['files'][name]['date'] = datetime.fromtimestamp(info.st_mtime, tz=timezone.utc).strftime('%Y-%m-%dT%TZ')
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
	fields=['path', 'url']
	for field in fields:
		if field in data and not data[field] is None:
			print(field + '\t' + str(data[field]))
	fields=['size', 'date']
	for field in fields:
		if field in data and not data[field] is None:
			print('WFX_SCRIPT_STR_' + field.upper() + '\t' + str(data[field]))
	print('Fs_PropsActs ' + act_copy)
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
