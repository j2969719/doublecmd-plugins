#!/bin/env python3

import os
import sys
import json
import calendar
import tempfile
import subprocess
from datetime import datetime, timezone

import time

verb = sys.argv[1]
env_var = 'DC_WFX_TP_SCRIPT_DATA'
msg = 'Range'
priority = {'0': 'emergency', '1': 'alert', '2': 'critical', '3': 'error', '4': 'warning', '5': 'notice', '6': 'info', '7': 'debug',}

def get_jsondata():
	try:
		with open(os.environ[env_var]) as f:
			obj = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	return obj


def get_priority_level():
	level = '4'
	scr = os.path.basename(__file__)
	conf = os.path.join(os.path.dirname(os.environ['COMMANDER_INI']), 'j2969719.json')
	try:
		with open(conf) as f:
			obj = json.loads(f.read())
			if scr in obj:
				level = obj[scr]
			f.close()
	except: pass
	return level

def vfs_init():
	tf = tempfile.NamedTemporaryFile(suffix='_errs.json', delete=False)
	filename = tf.name
	tf.close()
	print('Fs_Set_'+ env_var +' ' + filename)
	text = 'Fs_MultiChoice ' + msg + '\t'
	for key, name in priority.items():
		text = text + '\t' + name
	print(text)
	sys.exit()

def vfs_setopt(option, value):
	if option == msg:
		scr = os.path.basename(__file__)
		conf = os.path.join(os.path.dirname(os.environ['COMMANDER_INI']), 'j2969719.json')
		level = '4'
		for key, name in priority.items():
			if name == value:
				level = key
		try:
			with open(conf) as f:
				obj = json.loads(f.read())
				obj[scr] = level
				f.close()
		except FileNotFoundError:
			obj = json.loads('{ ' + scr + ' : "'+ level + '", }')
		with open(conf, 'w') as f:
			json.dump(obj, f)
			f.close()
	sys.exit()

def vfs_list(path):
	data = {}
	level = '--priority=' + get_priority_level()
	out = subprocess.check_output(['journalctl', '-b', '0', level, '--output=json']) #"warning",
	for line in out.splitlines():
		obj = json.loads(line)
		text = "".join(c for c in str(obj['MESSAGE']) if (c.isalnum() or c in '.-_ '))[:200] + ' ('+  obj['__MONOTONIC_TIMESTAMP'] + ')'
		if 'PRIORITY' in obj:
			text = text + '.' + priority[obj['PRIORITY']]
		print('----------\t' + datetime.fromtimestamp(float(obj['__REALTIME_TIMESTAMP'][:-6] + '.' + obj['__REALTIME_TIMESTAMP'][-6:]), tz=timezone.utc).strftime('%Y-%m-%dT%TZ') + '\t-1\t' + text)
		data[text] = obj
	try:
		with open(os.environ[env_var], 'w') as f:
			json.dump(data, f)
			f.close()
	except: pass
	sys.exit()

def vfs_execute(filename):
	data = get_jsondata()
	print('Fs_Info_Message ' + data[filename[1:]]['MESSAGE'])
	sys.exit()

def vfs_properties(filename):
	data = get_jsondata()
	props = ('PRIORITY', 'MESSAGE', '_HOSTNAME', 'SYSLOG_IDENTIFIER', '_TRANSPORT')
	for prop in props:
		if prop in data[filename[1:]] and not data[filename[1:]][prop] is None:
			if prop == 'PRIORITY':
				print('filetype\t' + priority[data[filename[1:]][prop]].capitalize())
			else:
				name = prop.replace('_', ' ').strip().capitalize()
				print(name + '\t' + data[filename[1:]][prop])
	sys.exit()

def vfs_deinit():
	os.remove(os.environ[env_var])
	sys.exit()

def vfs_getfile(src, dst):
	for i in range(100):
		print(i)
		#time.sleep(1)
	sys.exit()

if verb == 'init':
	vfs_init()
elif verb == 'setopt':
	vfs_setopt(sys.argv[2], sys.argv[3])
elif verb == 'list':
	vfs_list(sys.argv[2])
elif verb == 'copyout':
	vfs_getfile(sys.argv[2], sys.argv[3])
elif verb == 'run':
	vfs_execute(sys.argv[2])
elif verb == 'properties':
	vfs_properties(sys.argv[2])
elif verb == 'localname':
	vfs_localname(sys.argv[2])
elif verb == 'deinit':
	vfs_deinit()

sys.exit(1)
