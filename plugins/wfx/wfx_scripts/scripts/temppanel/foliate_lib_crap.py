#!/bin/env python3

import os
import sys
import json
import stat
import tempfile
import calendar
try:
	import send2trash
except:
	pass
from urllib.parse import quote, unquote, urlparse
from datetime import datetime, timezone
from shutil import copyfile

verb = sys.argv[1]
envvar = 'DC_WFX_TP_SCRIPT_DATA'
foliate_dir = '/.local/share/com.github.johnfactotum.Foliate/'
props = ('creator', 'title', 'publisher', 'description')

def get_jsonobj():
	try:
		with open(os.environ[envvar]) as f:
			obj = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	return obj

def vfs_init():
	tf = tempfile.NamedTemporaryFile(suffix='_foliate.json', delete=False)
	filename = tf.name
	tf.close()
	print('Fs_Set_'+ envvar +' ' + filename)
	home_dir = os.environ['HOME']
	uri_store = home_dir + foliate_dir + 'library/uri-store.json'
	try:
		with open(uri_store) as f:
			store = json.loads(f.read())
			f.close()
	except:
		sys.exit(1)
	res = json.loads('{}')
	for element in store['uris']:
		book_info = home_dir + foliate_dir + quote(element[0]) + '.json'
		try:
			with open(book_info) as f:
				info = json.loads(f.read())
				f.close()
		except Exception as e:
			print(e, file=sys.stderr)
			sys.exit(1)
		autor = info['metadata']['creator']
		if len(autor) > 80:
			autor = info['metadata']['creator'][:80] + '...'
		title = info['metadata']['title']
		if len(title) > 80:
			title = info['metadata']['title'][:80] + '...'
		rawname = autor + ' - ' + title
		ext = os.path.splitext(element[1])[1]
		tmp_name = "".join(c for c in rawname.lstrip() if c != "/")
		extra = ''
		i = 1
		name = tmp_name + ext
		while name in res:
			name = tmp_name + ' (' + str(i) + ')' + ext
			i += 1
		res[name] = {}
		res[name]['path'] = unquote(urlparse(element[1]).path)
		res[name]['id'] = element[0]
		for prop in props:
			if prop in info['metadata'] and info['metadata'][prop] != '':
				res[name][prop] = info['metadata'][prop]
		if 'progress' in info:
			res[name]['progress'] = int(info['progress'][0] * 100 / info['progress'][1])
	try:
		with open(filename, 'w') as f:
			json.dump(res, f)
			f.close()
	except FileNotFoundError:
		pass
	sys.exit()

def vfs_setopt(option, value):
	sys.exit(1)

def vfs_list(path):
	obj = get_jsonobj()
	for name in obj:
		try:
			info = os.stat(obj[name]['path'])
			print(stat.filemode(info.st_mode) + '\t' + datetime.fromtimestamp(info.st_mtime, tz=timezone.utc).strftime('%Y-%m-%dT%TZ') + '\t' + str(info.st_size) + '\t' + name)
		except:
			print('----------\t0000-00-00 00:00:00 \t-\t' + name)
	sys.exit()

def vfs_getfile(src, dst):
	obj = get_jsonobj()
	localname = obj[src[1:]]['path']
	if not localname is None:
		copyfile(localname, dst, follow_symlinks=False)
		sys.exit()
	sys.exit(1)

def vfs_rmfile(filename):
	obj = get_jsonobj()
	localname = obj[filename[1:]]['path']
	if not localname is None:
		send2trash.send2trash(localname)
		sys.exit()
	sys.exit(1)

def vfs_properties(filename):
	obj = get_jsonobj()
	print('path\t' + obj[filename[1:]]['path'])
	for prop in props:
		if prop in obj[filename[1:]] and not obj[filename[1:]][prop] is None:
			print(prop.capitalize() + '\t' + obj[filename[1:]][prop])
	if 'progress' in obj[filename[1:]] and not obj[filename[1:]]['progress'] is None:
		print('Progress\t' + str(obj[filename[1:]]['progress']) + '%')
	sys.exit()

def vfs_localname(filename):
	obj = get_jsonobj()
	localname = obj[filename[1:]]['path']
	if not localname is None:
		print(localname)
		sys.exit()
	sys.exit(1)

def vfs_deinit():
	os.remove(os.environ[envvar])
	sys.exit()


if verb == 'init':
	vfs_init()
elif verb == 'setopt':
	vfs_setopt(sys.argv[2], sys.argv[3])
elif verb == 'list':
	vfs_list(sys.argv[2])
elif verb == 'copyout':
	vfs_getfile(sys.argv[2], sys.argv[3])
elif verb == 'rm':
	vfs_rmfile(sys.argv[2])
elif verb == 'run':
	vfs_execute(sys.argv[2])
elif verb == 'properties':
	vfs_properties(sys.argv[2])
elif verb == 'localname':
	vfs_localname(sys.argv[2])
elif verb == 'deinit':
	vfs_deinit()

sys.exit(1)
