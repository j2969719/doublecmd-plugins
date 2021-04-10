#!/bin/env python

import os
import sys
import stat
import calendar
from datetime import datetime, timezone
from shutil import copyfile

filelist = "/tmp/doublecmd_filelist.lst"
verb = sys.argv[1]

def vfs_filelist(path):
	f = open(filelist)
	for line in f:
		try:
			info = os.stat(line[:-1])
			print(stat.filemode(info.st_mode) + "\t" + datetime.fromtimestamp(info.st_mtime, tz=timezone.utc).strftime('%Y-%m-%dT%TZ') + "\t" + str(info.st_size) + "\t" + line[:-1])
		except FileNotFoundError:
			pass
	f.close()
	sys.exit()

def vfs_exists(path):
	try:
		f = open(filelist, 'r')
	except FileNotFoundError:
		return False;
	for line in f:
		if line.strip("\n") == path:
			f.close()
			return True;
	f.close()
	return False;


def vfs_putfile(path):
	if (vfs_exists(path) == True):
		sys.exit()

	f = open(filelist, 'a')
	f.write(path)
	f.write("\n")
	f.close()
	sys.exit()

def vfs_remove(path):
	with open(filelist, "r+") as f:
		lines = f.readlines()
		f.seek(0)
		for line in lines:
			if line.strip("\n") != path:
				f.write(line)
		f.truncate()
		f.close()
	sys.exit()


if verb == "list":
	vfs_filelist(sys.argv[2])
elif verb == "copyin":
	vfs_putfile(sys.argv[2])
elif verb == "exists":
	if vfs_exists(sys.argv[2]):
		sys.exit()
elif verb == "copyout":
	copyfile(sys.argv[2][1:], sys.argv[3].replace(sys.argv[2], "/") + os.path.basename(sys.argv[2]))
	sys.exit()
elif verb == "rm":
	vfs_remove(sys.argv[2][1:])
elif verb == "mkdir":
	sys.exit()
elif verb == "chmod":
	os.chmod(sys.argv[2][1:], int(sys.argv[3], base=8))
	sys.exit()
elif verb == "modtime":
	date = datetime.fromisoformat(sys.argv[3][:-1] + "+00:00")
	modtime = calendar.timegm(date.timetuple())
	os.utime(sys.argv[2][1:], (modtime, modtime))
	sys.exit()
elif verb == "localname":
	print(sys.argv[2][1:])
	sys.exit()
elif verb == "run":
	os.system("xdg-open \"" + sys.argv[2][1:] + "\"")
	sys.exit()
elif verb == "getfields":
	print("basename")
	print("dirname")
	sys.exit()
elif verb == "getvalue":
	if sys.argv[2] == "basename":
		print(os.path.basename(sys.argv[3][1:]))
	elif sys.argv[2] == "dirname":
		print(os.path.dirname(sys.argv[3][1:]))
	sys.exit()

sys.exit(1)
