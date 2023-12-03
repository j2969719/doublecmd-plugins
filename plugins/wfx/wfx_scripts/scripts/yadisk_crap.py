#!/bin/env python3

# pip install yadisk

# https://www.youtube.com/watch?v=c9Yl9B5cP6U
# tovarisch major was already here

import os
import sys
import json
import tempfile
import yadisk
from yadisk import YaDisk

verb = sys.argv[1]
scr = os.path.basename(__file__)
auth_token = None
try:
	auth_token = os.environ['DC_WFX_SCRIPT_DATA']
except KeyError:
	pass
conf = os.path.join(os.path.dirname(os.environ['COMMANDER_INI']), 'j2969719.json')
try:
	with open(conf) as f:
		obj = json.loads(f.read())
		if not scr in obj:
			obj[scr] = {}
			obj[scr]['app_id'] = None
			obj[scr]['secret'] = None
			obj[scr]['connections'] = {}
		f.close()
except FileNotFoundError:
	obj = json.loads('{ ' + scr + ' : { "connections" : {}, "app_id" : null, "secret" : null } }')
	with open(conf, 'w') as f:
		json.dump(obj, f)
		f.close()
y = YaDisk(id=obj[scr]['app_id'], secret=obj[scr]['secret'], token=auth_token)

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
	if not 'app_id' in obj[scr] or obj[scr]['app_id'] is None:
		print('client_id')
	if not 'secret' in obj[scr] or obj[scr]['secret'] is None:
		print('secret')
	
	if 'connections' in obj[scr] and obj[scr]['connections'] != {}:
		init_str = 'Fs_MultiChoice Connection\t'
		for element in obj[scr]['connections']:
			init_str = init_str + str(element) + '\t'
		init_str = init_str + '<ADD>'
		print(init_str)
	else:
		print('Fs_PushValue Connection\t<ADD>')
	sys.exit()

def vfs_setopt(option, value):
	if value == '':
		sys.exit(1)
	elif option == 'client_id':
		obj[scr]['app_id'] = value
		save_obj()
		sys.exit()
	elif option == 'secret':
		obj[scr]['secret'] = value
		save_obj()
		sys.exit()
	elif option == 'Connection':
		if value == '<ADD>':
			print('Fs_Request_Options')
			print('Name')
			print("Fs_Info_Message Authenticate in your browser and copy the code.")
			print('Fs_Open %s' % y.get_code_url())
			print('Code')
			sys.exit()
		elif not value is None:
			if value in obj[scr]['connections']:
				print('Fs_Set_DC_WFX_SCRIPT_DATA ' + obj[scr]['connections'][value]['token'])
			sys.exit()
	elif option == 'Name':
		obj[scr]['pending'] = value
		save_obj()
		sys.exit()
	elif option == 'Code':
		if not 'pending' in obj[scr]:
			sys.exit(1)
		try:
			r = y.get_token(value)
		except yadisk.exceptions.BadRequestError:
			print("Fs_Info_Message Bad code")
			del obj[scr]['pending']
			save_obj()
			sys.exit(1)
		y.token = r.access_token
		if y.check_token() == True:
			name = obj[scr]['pending']
			if not name is None:
				obj[scr]['connections'][name] = {}
				obj[scr]['connections'][name]['token'] = r.access_token
		del obj[scr]['pending']
		save_obj()
		print('Fs_Set_DC_WFX_SCRIPT_DATA ' + r.access_token)
		sys.exit()
	elif option == 'Publish':
		y.publish(value)
	elif option == 'Unpublish':
		y.unpublish(value)
	sys.exit(1)

def vfs_list(path):
	
	if y.check_token() == True:
		ls = y.listdir(path, fields=['name', 'type', 'size', 'modified'])
		for f in ls:
			if f['type'] == 'file':
				print('----------\t' + f['modified'].strftime('%Y-%m-%dT%TZ') + '\t' + str(f['size']) + '\t' + f['name'])
			else:
				print('d---------\t' + f['modified'].strftime('%Y-%m-%dT%TZ') + '\t-\t' + f['name'])
		sys.exit()
	sys.exit(1)

def vfs_getfile(src, dst):
	if y.check_token() == True:
		y.download(src, dst)
		sys.exit()
	sys.exit(1)

def vfs_exists(path):
	if y.check_token() and y.exists(path):
		sys.exit()
	sys.exit(1)

def vfs_putfile(src, dst):
	if y.check_token() == True:
		y.upload(src, dst, overwrite=True)
		sys.exit()
	sys.exit(1)

def vfs_rmfile(path):
	if y.check_token() == True:
		y.remove(path)
		sys.exit()
	sys.exit(1)

def vfs_cpfile(src, dst):
	if y.check_token() == True:
		y.copy(src, dst, overwrite=True)
		sys.exit()
	sys.exit(1)

def vfs_mvfile(src, dst):
	if y.check_token() == True:
		y.move(src, dst, overwrite=True)
		sys.exit()
	sys.exit(1)

def vfs_mkdir(path):
	if y.check_token() == True:
		y.mkdir(path)
		sys.exit()
	sys.exit(1)

def vfs_rmdir(path):
	if y.check_token() == True:
		y.remove(path)
		sys.exit()
	sys.exit(1)

def vfs_properties(path):
	meta = y.get_meta(path, fields=['antivirus_status', 'type', 'path', 'exif', 'created', 'modified', 'custom_properties', 'public_url'])
	for field in meta:
		if not meta[field] is None:
			if field == 'public_url':
				print('url\t' + str(meta[field]))
			elif field == 'type':
				if meta[field] == 'dir':
					print('content_type\tinode/directory')
			else:
				print(field.replace('_', ' ').capitalize() + '\t' + str(meta[field]))
	print('Fs_PropsActs Publish\tUnpublish')
	info = y.get_disk_info(fields=['total_space', 'used_space'])
	for field in info:
		if not info[field] is None:
			print('YaDisk ' + field.replace('_', ' ') + '\t' + size_str(info[field]))
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
elif verb == 'cp':
	vfs_cpfile(sys.argv[2], sys.argv[3])
elif verb == 'mv':
	vfs_mvfile(sys.argv[2], sys.argv[3])
elif verb == 'rm':
	vfs_rmfile(sys.argv[2])
elif verb == 'mkdir':
	vfs_mkdir(sys.argv[2])
elif verb == 'rmdir':
	vfs_rmdir(sys.argv[2])
elif verb == 'properties':
	vfs_properties(sys.argv[2])

sys.exit(1)
