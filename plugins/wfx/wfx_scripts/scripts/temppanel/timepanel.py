#!/bin/env python3

import os
import sys
import json
import stat
import calendar
from datetime import datetime, timezone
from shutil import copyfile

verb = sys.argv[1]
conf = 'timepanel.json'
env_var = 'DC_WFX_TP_SCRIPT_DATA'


def get_jsonobj():
	try:
		with open(os.environ[env_var]) as f:
			obj = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	return obj

def save_jsonobj(obj):
	try:
		with open(os.environ[env_var], 'w') as f:
			json.dump(obj, f)
			f.close()
	except FileNotFoundError:
		pass

def get_fileobj(obj, path):
	dirname = os.path.dirname(path)
	name = os.path.basename(path)
	return obj[dirname][name]

def add_fileobj(obj, src, dst, is_dir = False):
	dirname = os.path.dirname(dst)
	name = os.path.basename(dst)
	if not dirname in obj:
		obj[dirname] = {}
	if not name in obj[dirname]:
		obj[dirname][name] = {}
	if is_dir == True:
		obj[dirname][name]['dir'] = True
		obj[dirname][name]['localname'] = None
	else:
		obj[dirname][name]['localname'] = src
	obj[dirname][name]['created'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

def rm_fileobj(obj, path):
	if path in obj:
		del obj[path]
	dirname = os.path.dirname(path)
	name = os.path.basename(path)
	if obj[dirname][name] is None:
		sys.exit(1)
	else:
		del obj[dirname][name]

def get_localname(obj, filename):
	element = get_fileobj(obj, filename)
	if not element is None:
		return element['localname']
	return None



def vfs_init():
	dc_ini = os.environ['COMMANDER_INI']
	filename = os.path.join(os.path.dirname(dc_ini), conf)
	try:
		with open(filename) as f:
			obj = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		obj = json.loads('{}')
		try:
			with open(filename, 'w') as f:
				json.dump(obj, f)
				f.close()
		except FileNotFoundError:
			pass
	print('Fs_Set_'+ env_var +' ' + filename)
	sys.exit()

def vfs_setopt(option, value):
	sys.exit(1)

def vfs_list(path):
	obj = get_jsonobj()
	if len(path) > 1 and path[-1] == '/':
		path = path[:-1]
	if path not in obj:
		sys.exit()
	for name in obj[path].keys():
		if not 'dir' in obj[path][name]:
			try:
				info = os.stat(obj[path][name]['localname'])
				print(stat.filemode(info.st_mode) + '\t' + obj[path][name]['created'] + '\t' + str(info.st_size) + '\t' + name)
			except:
				print('----------\t' + obj[path][name]['created'] + '\t-\t' + name)
		else:
			print('drwxr-xr-x\t' + obj[path][name]['created'] + '\t-\t' + name)
	sys.exit()

def vfs_getfile(src, dst):
	obj = get_jsonobj()
	localname = get_localname(obj, src)
	if not localname is None:
		copyfile(localname, dst, follow_symlinks=False)
		sys.exit()
	sys.exit(1)

def vfs_exists(path):
	obj = get_jsonobj()
	try:
		element = get_fileobj(obj, path)
	except:
		sys.exit(1)
	if element is None:
		sys.exit(1)
	sys.exit()

def vfs_putfile(src, dst):
	obj = get_jsonobj()
	add_fileobj(obj, src, dst)
	save_jsonobj(obj)
	sys.exit()

def vfs_rmfile(filename):
	obj = get_jsonobj()
	rm_fileobj(obj, filename)
	save_jsonobj(obj)
	sys.exit()

def vfs_cpfile(src, dst):
	obj = get_jsonobj()
	localname = get_localname(obj, src)
	if not localname is None:
		add_fileobj(obj, localname, dst)
	save_jsonobj(obj)
	sys.exit()

def vfs_mvfile(src, dst):
	obj = get_jsonobj()
	name = os.path.basename(dst)
	olddir = os.path.dirname(src)
	newdir = os.path.dirname(dst)
	oldname = os.path.basename(src)
	localname = get_localname(obj, src)
	localdir = os.path.dirname(localname)
	if not localname is None:
		newlocalname = localdir + '/' + name
		os.rename(localname, newlocalname)
		add_fileobj(obj, localname, dst)
		rm_fileobj(obj, src)
		obj[newdir][name]['localname'] = newlocalname
	else:
		obj[dst] = obj[src]
		if not newdir in obj:
			obj[newdir] = {}
		obj[newdir][name] = {}
		obj[newdir][name] = obj[olddir][oldname]
		del obj[olddir][oldname]
		del obj[src]
	save_jsonobj(obj)
	sys.exit()

def vfs_mkdir(path):
	name = os.path.basename(path)
	obj = get_jsonobj()
	add_fileobj(obj, name, path, is_dir = True)
	if not path in obj:
		obj[path] = {}
	save_jsonobj(obj)
	sys.exit()

def vfs_rmdir(path):
	obj = get_jsonobj()
	rm_fileobj(obj, path)
	save_jsonobj(obj)
	sys.exit()

def vfs_execute(filename):
	obj = get_jsonobj()
	localname = get_localname(obj, filename)
	if not localname is None:
		print('Fs_Open ' + localname)
	sys.exit()

def vfs_properties(filename):
	obj = get_jsonobj()
	localname = get_localname(obj, filename)
	if not localname is None:
		print('Path\t' + localname)
		try:
			info = os.stat(localname)
			print(f'WFX_SCRIPT_STR_SIZE\t{info.st_size:,}')
			print('WFX_SCRIPT_STR_MODE\t' + stat.filemode(info.st_mode))
			print('WFX_SCRIPT_STR_MTIME\t' + datetime.fromtimestamp(info.st_mtime, tz=timezone.utc).strftime('%Y-%m-%d %T') )
		except:
			print(' \tWFX_SCRIPT_STR_MISSING')
	element = get_fileobj(obj, filename)
	if not element is None:
		print('WFX_SCRIPT_STR_CTIME\t' + element['created'])
	sys.exit()

def vfs_chmod(filename, newmode):
	obj = get_jsonobj()
	localname = get_localname(obj, filename)
	if not localname is None:
		os.chmod(localname, int(newmode, base=8))
		sys.exit()
	sys.exit(1)

def vfs_utime(filename, newdate):
	obj = get_jsonobj()
	localname = get_localname(obj, filename)
	if not localname is None:
		newdatetime = datetime.fromisoformat(newdate[:-1] + '+00:00')
		modtime = calendar.timegm(newdatetime.timetuple())
		os.utime(localname, (modtime, modtime))
	sys.exit()

def vfs_localname(filename):
	obj = get_jsonobj()
	localname = get_localname(obj, filename)
	if not localname is None:
		print(localname)
		sys.exit()
	sys.exit(1)

def vfs_deinit():
	dc_ini = os.environ['COMMANDER_INI']
	filename = os.path.join(os.path.dirname(dc_ini), conf)
	os.remove(filename)
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
elif verb == 'run':
	vfs_execute(sys.argv[2])
elif verb == 'properties':
	vfs_properties(sys.argv[2])
elif verb == 'chmod':
	vfs_chmod(sys.argv[2], sys.argv[3])
elif verb == 'localname':
	vfs_localname(sys.argv[2])
elif verb == 'deinit':
	vfs_deinit()

sys.exit(1)
