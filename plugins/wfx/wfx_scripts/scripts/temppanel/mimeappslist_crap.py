#!/bin/env python3

import os
import gi
import sys
import stat
import calendar
from datetime import datetime, timezone
from shutil import copyfile
from gi.repository import Gio, GLib

verb = sys.argv[1]

def log_err(text):
	print(text, file=sys.stderr)

def get_mimeappsfilename():
	return os.environ['HOME'] + '/.config/mimeapps.list'

def get_appinfo(appid):
	appinfo = None
	try:
		appinfo = Gio.DesktopAppInfo.new(appid)
	except: pass
	return appinfo

def get_appinfo_from_path(path):
	names = path.split('/')
	if (len(names) > 3):
		return get_appinfo(names[3])
	return None

def open_mimeappslist():
	filename = get_mimeappsfilename()
	keyfile = GLib.KeyFile()
	keyfile.load_from_file(filename, 0)
	return keyfile

def vfs_init():
	copyfile(get_mimeappsfilename(), '/tmp/mimeapps.list.bak')
	print('Fs_Info_Message Backup copy created in /tmp/mimeapps.list.bak')

def vfs_list(path):
	mimeapps = open_mimeappslist()
	if (path == '/'):
		groups = mimeapps.get_groups()
		for group in groups[0]:
			print('dr-x------\t0000-00-00 00:00:00\t-\t' + group)
	else:
		names = path.split('/')
		if (len(names) == 2):
			keys = mimeapps.get_keys(names[1])
			for mime in keys[0]:
				name = mime.replace('/', '|')
				print('drwx------\t0000-00-00 00:00:00\t-\t' + name)
		elif (len(names) == 3):
			key = names[2].replace('|', '/')
			apps = mimeapps.get_string_list(names[1], key)
			for app in apps:
				appinfo = get_appinfo(app)
				if appinfo is not None:
					try:
						info = os.lstat(appinfo.get_filename())
						mode = stat.filemode(info.st_mode)
						filetime = datetime.fromtimestamp(info.st_mtime, tz=timezone.utc).strftime('%Y-%m-%dT%TZ')
						print(mode + '\t' + filetime + '\t' + str(info.st_size) + '\t' + app)
					except:
						print('----------\t0000-00-00 00:00:00\t-\t' + app)
				else:
					print('----------\t0000-00-00 00:00:00\t-\t' + app)
	mimeapps.unref()
	sys.exit()

def vfs_getfile(src, dst):
	appinfo = get_appinfo_from_path(src)
	if appinfo is not None:
		copyfile(appinfo.get_filename(), dst, follow_symlinks=False)
		sys.exit()
	sys.exit(1)

def vfs_putfile(src, dst):
	appinfo = get_appinfo_from_path(src)
	if appinfo is not None:
		names = dst.split('/')
		if (len(names) > 3):
			mimeapps = open_mimeappslist()
			key = names[2].replace('|', '/')
			app = os.path.basename(dst)
			apps = mimeapps.get_string_list(names[1], key)
			if app not in apps:
				apps.append(app)
			mimeapps.set_string_list(names[1], key, apps)
			filename = get_mimeappsfilename()
			mimeapps.save_to_file(filename)
			mimeapps.unref()
			sys.exit()
	sys.exit(1)

def vfs_rmfile(filename):
	names = filename.split('/')
	if (len(names) == 4):
		mimeapps = open_mimeappslist()
		key = names[2].replace('|', '/')
		apps = mimeapps.get_string_list(names[1], key)
		if names[3] in apps:
			apps.remove(names[3])
			mimeapps.set_string_list(names[1], key, apps)
			filename = get_mimeappsfilename()
			mimeapps.save_to_file(filename)
			mimeapps.unref()
			sys.exit()
		mimeapps.unref()
	sys.exit(1)

def vfs_rmdir(path):
	names = path.split('/')
	if (len(names) == 3):
		mimeapps = open_mimeappslist()
		key = names[2].replace('|', '/')
		mimeapps.remove_key(names[1], key)
		filename = get_mimeappsfilename()
		mimeapps.save_to_file(filename)
		mimeapps.unref()
		sys.exit()
	sys.exit(1)

def vfs_execute(path):
	appinfo = get_appinfo_from_path(path)
	if appinfo is not None:
		os.system('xdg-open "' + appinfo.get_filename() + '"')
	sys.exit()

def vfs_properties(path):
	names = path.split('/')
	if names[1] is not None:
		print('Group\t' + names[1])
	if (len(names) > 1):
		mime = names[2].replace('|', '/')
		print('content_type\t' + mime)
	if (len(names) > 3):
		appinfo = get_appinfo(names[3])
		if appinfo is not None:
			print(appinfo.get_filename())
			print('Path\t' + appinfo.get_filename())
			
	sys.exit()

def vfs_localname(path):
	appinfo = get_appinfo_from_path(path)
	if appinfo is not None:
		print(appinfo.get_filename())
		sys.exit()
	sys.exit(1)


if verb == 'init':
	vfs_init()
elif verb == 'list':
	vfs_list(sys.argv[2])
elif verb == 'copyin':
	vfs_putfile(sys.argv[2], sys.argv[3])
elif verb == 'copyout':
	vfs_getfile(sys.argv[2], sys.argv[3])
elif verb == 'rm':
	vfs_rmfile(sys.argv[2])
elif verb == 'rmdir':
	vfs_rmdir(sys.argv[2])
elif verb == 'run':
	vfs_execute(sys.argv[2])
elif verb == 'properties':
	vfs_properties(sys.argv[2])
elif verb == 'localname':
	vfs_localname(sys.argv[2])

sys.exit(1)
