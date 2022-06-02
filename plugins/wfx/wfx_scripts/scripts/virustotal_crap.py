#!/bin/env python3

# pip install virustotal-api

import os
import sys
import json
import requests
import calendar
import hashlib

from datetime import datetime, timezone
from virus_total_apis import PublicApi as VirusTotalPublicApi

verb = sys.argv[1]
scr = os.path.basename(__file__)
conf = os.path.join(os.path.dirname(os.environ['COMMANDER_INI']), 'j2969719.json')
try:
	with open(conf) as f:
		obj = json.loads(f.read())
		if not scr in obj:
			obj[scr] = {}
			obj[scr]['api_key'] = None
			obj[scr]['files'] = {}
		f.close()
except FileNotFoundError:
	obj = json.loads('{ ' + scr + ' : { "files" : {}, "api_key" : null }')
	with open(conf, 'w') as f:
		json.dump(obj, f)
		f.close()
try:
	vt = VirusTotalPublicApi(obj[scr]['api_key'])
except:
	pass

def save_obj():
	with open(conf, 'w') as f:
		json.dump(obj, f)
		f.close()

def get_report(src):
	name = os.path.basename(src)
	data = obj[scr]['files'][name]
	try:
		response = vt.get_file_report(data['sha1'])
	except Exception as e:
			print(str(e), file=sys.stderr)
			sys.exit(1)
	if not 'results' in response:
		if 'error' in response:
			print(response['error'], file=sys.stderr)
		return response
	if 'scan_date' in response['results']:
		obj[scr]['files'][name]['date'] = response['results']['scan_date']
		if 'positives' in response['results']:
			obj[scr]['files'][name]['size'] = str(response['results']['positives'])
		save_obj()
	if response['response_code'] != 200:
		response['results']['show_msgdlg'] = True
	return response['results']


def vfs_init():
	print('Fs_Request_Options')
	if not 'api_key' in obj[scr] or obj[scr]['api_key'] is None:
		print('api_key')
	sys.exit()

def vfs_setopt(option, value):
	if value == '':
		sys.exit(1)
	elif option == 'api_key':
		obj[scr]['api_key'] = value
		save_obj()
		sys.exit()
	sys.exit(1)

def vfs_list(path):
	if path == '/' and not obj[scr]['files'] is None:
		for name in obj[scr]['files'].keys():
			if not 'size' in obj[scr]['files'][name] or obj[scr]['files'][name]['size'] is None:
				size = '-'
			else:
				size = obj[scr]['files'][name]['size']
			if not 'date' in obj[scr]['files'][name] or obj[scr]['files'][name]['date'] is None:
				date = '0000-00-00 00:00:00'
			else:
				date = obj[scr]['files'][name]['date']
			print('----------\t' + date +'\t' + size + '\t' + name)
		sys.exit()
	sys.exit(1)

def vfs_getfile(src, dst):
	data = get_report(src)
	if not 'scans' in data:
		sys.exit(1)
	with open(dst, 'w') as f:
		f.write('Antivirus\tVersion\tUpdate\tDetected\n')
		for av in data['scans'].keys():
			if not data['scans'][av]['version'] is None:
				ver = str(data['scans'][av]['version'])
			else:
				ver = ' '
			if not data['scans'][av]['update'] is None:
				upd = str(data['scans'][av]['update'])
			else:
				upd = ' '
			if not data['scans'][av]['result'] is None:
				result = data['scans'][av]['result']
			else:
				result = ' '
			line = av + '\t' + ver + '\t' + upd + '\t' + result + '\n'
			f.write(line)
		f.close()
		sys.exit()
	sys.exit(1)

def vfs_exists(path):
	if os.path.basename(path) in obj[scr]['files']:
		sys.exit()
	sys.exit(1)

def vfs_putfile(src, dst):
	name = os.path.basename(dst) + '_report.tsv'
	info = os.stat(src)
	if info.st_size > 200000000:
		sys.exit(1)
	sha1 = hashlib.sha1(open(src, 'rb').read()).hexdigest()
	try:
		response = vt.get_file_report(sha1)
	except Exception as e:
			print(str(e), file=sys.stderr)
			sys.exit(1)
	if 'results' in response:
		obj[scr]['files'][name] = {}
		obj[scr]['files'][name]['sha1'] = sha1
		if 'scan_date' in response['results']:
			obj[scr]['files'][name]['date'] = response['results']['scan_date']
		else:
			vt.scan_file(src)
		if 'positives' in response['results']:
			obj[scr]['files'][name]['size'] = str(response['results']['positives'])
		save_obj()
		sys.exit()
	sys.exit(1)

def vfs_rmfile(path):
	del obj[scr]['files'][os.path.basename(path)]
	save_obj()
	sys.exit()

def vfs_properties(path):
	data = get_report(path)
	if 'error' in data:
		print('Fs_Info_Message ' + data['error'])
		sys.exit()
	if 'show_msgdlg' in data or data['response_code'] < 0:
		if not data['verbose_msg'] is None:
			print('Fs_Info_Message ' + data['verbose_msg'])
			sys.exit()
	fields=['scan_date', 'total', 'positives']
	for field in fields:
		if field in data and not data[field] is None:
			print(field.replace('_', ' ').capitalize() + '\t' + str(data[field]))
	if 'permalink' in data and not data['permalink'] is None:
		print('url\t' + data['permalink'])
	if 'sha1' in data and not data['sha1'] is None:
		print('SHA1\t' + data['sha1'])
	sys.exit()

def vfs_deinit():
	obj[scr]['files'] = {}
	save_obj()
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
elif verb == 'deinit':
	vfs_deinit()

sys.exit(1)
