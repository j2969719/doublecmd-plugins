#!/bin/env python3

import os
import sys
import json
import base64
import requests
import calendar
import gi

from datetime import datetime, timezone
from base64 import b64encode

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

verb = sys.argv[1]
scr = os.path.basename(__file__)
act_open = 'Open in Browser'
act_copy = 'Copy URL to Clipboard'
conf = os.path.join(os.path.dirname(os.environ['COMMANDER_INI']), 'j2969719.json')
try:
	with open(conf) as f:
		obj = json.loads(f.read())
		if not scr in obj:
			obj[scr] = {}
			obj[scr]['files'] = {}
		f.close()
except FileNotFoundError:
	obj = json.loads('{ ' + scr + ' : { "files" : {}, "api_key" : null, "client_id" : null} }')
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
	if not 'api_key' in obj[scr] or obj[scr]['api_key'] is None:
		print('api_key')
	if not 'client_id' in obj[scr] or obj[scr]['client_id'] is None:
		print('client_id')
	sys.exit()

def vfs_setopt(option, value):
	if value == '':
		sys.exit(1)
	elif option == 'client_id':
		obj[scr]['client_id'] = value
		save_obj()
		sys.exit()
	elif option == 'api_key':
		obj[scr]['api_key'] = value
		save_obj()
		sys.exit()
	elif option == act_copy:
		url = obj[scr]['files'][os.path.basename(value)]['link']
		clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
		clipboard.set_text(url, -1)
		clipboard.store()
		sys.exit()
	elif option == act_open:
		print('Fs_Open https://www.imgur.com/' + obj[scr]['files'][os.path.basename(value)]['id'])
		sys.exit()
	sys.exit(1)

def vfs_list(path):
	if path == '/' and not obj[scr]['files'] is None:
		for name in obj[scr]['files'].keys():
			if not 'size' in obj[scr]['files'][name] or obj[scr]['files'][name]['size'] is None:
				size = '-'
			else:
				size = str(obj[scr]['files'][name]['size'])
			if not 'datetime' in obj[scr]['files'][name] or obj[scr]['files'][name]['datetime'] is None:
				date = '0000-00-00 00:00:00'
			else:
				date = datetime.fromtimestamp(obj[scr]['files'][name]['datetime'], tz=timezone.utc).strftime('%Y-%m-%dT%TZ')
			print('----------\t' + date +'\t' + size + '\t' + name)
		sys.exit()
	sys.exit(1)

def vfs_getfile(src, dst):
	data = obj[scr]['files'][os.path.basename(src)]
	size = int(data['size'])
	with open(dst, 'wb') as f:
		try:
			response = requests.get(data['link'], stream=True)
		except Exception as e:
			print(str(e))
			sys.exit(1)
		if response.status_code == 200:
			total = response.headers.get('content-length')
			if total is None:
				f.write(response.content)
			else:
				downloaded = 0
				for data in response.iter_content(chunk_size=4096):
					downloaded += len(data)
					f.write(data)
					percent = int(downloaded * 100 / size)
					print(percent)
					sys.stdout.flush()
		else:
			print(str(response.status_code) + ": " + response.reason, file=sys.stderr)
		f.close()
		sys.exit()
	sys.exit(1)

def vfs_putfile(src, dst):
	url = 'https://api.imgur.com/3/upload.json'
	headers = {"Authorization": "Client-ID " + obj[scr]['client_id']}
	try:
		response = requests.post(url, headers = headers, data = {
			'key': obj[scr]['api_key'], 
			'image': b64encode(open(src, 'rb').read()),
			'type': 'base64',
			'name': os.path.basename(dst),
		})
	except Exception as e:
			print(str(e), file=sys.stderr)
			sys.exit(1)
	if response.status_code == 200:
			data = response.json()
			name = os.path.basename(data['data']['link'])
			data['data']['path'] = src
			obj[scr]['files'][name]= data['data']
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
	if 'size' in data and not data['size'] is None:
		print('Size\t' + size_str(data['size']))
	if 'link' in data and not data['link'] is None:
		print('url\t' + str(data['link']))
	if 'type' in data and not data['type'] is None:
		print('content_type\t' + str(data['type']))
	if 'datetime' in data and not data['datetime'] is None:
		date = datetime.fromtimestamp(data['datetime']).strftime('%Y-%m-%d %T')
		print('Created\t' + date)
	fields=['path', 'width', 'height']
	for field in fields:
		if field in data and not data[field] is None:
			print(field.replace('_', ' ').capitalize() + '\t' + str(data[field]))
	print('Fs_PropsActs ' + act_copy + '\t' + act_open)

	url = 'https://api.imgur.com/3/image/' + data['id']
	headers = {"Authorization": "Client-ID " + obj[scr]['client_id']}
	try:
		response = requests.get(url, headers = headers)
	except:
		pass
	if response.status_code == 200:
		data = response.json()['data']
		fields=['views', 'description', 'section']
		for field in fields:
			if field in data and not data[field] is None:
				print(field.replace('_', ' ').capitalize() + '\t' + str(data[field]))

	sys.exit()

if verb == 'init':
	vfs_init()
elif verb == 'setopt':
	vfs_setopt(sys.argv[2], sys.argv[3])
elif verb == 'list':
	vfs_list(sys.argv[2])
elif verb == 'copyout':
	vfs_getfile(sys.argv[2], sys.argv[3])
elif verb == 'copyin':
	vfs_putfile(sys.argv[2], sys.argv[3])
elif verb == 'rm':
	vfs_rmfile(sys.argv[2])
elif verb == 'properties':
	vfs_properties(sys.argv[2])

sys.exit(1)
