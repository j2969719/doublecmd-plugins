#!/bin/env python3

import os
import sys
import json
import tempfile
import requests
from requests.exceptions import HTTPError

verb = sys.argv[1]
envvar = 'DC_WFX_SCRIPT_DATA'


def get_dir_contents(obj, path):
	try:
		response = requests.get('https://api.github.com/repos/' + obj['repo'] + '/contents' + path)
	except Exception as e:
		print(str(e), file=sys.stderr)
		sys.exit(1)
	if response.status_code == 200:
		obj['contents'][path] = response.json()
		return obj
	else:
		print(str(response.status_code) + ": " + response.reason,file=sys.stderr)
		sys.exit(1)

def vfs_init():
	tf = tempfile.NamedTemporaryFile(suffix='_github.json', delete=False)
	filename = tf.name
	tf.close()
	print('Fs_Set_'+ envvar +' ' + filename)
	print('Fs_Request_Options')
	print('user/repo')
	sys.exit()

def vfs_setopt(option, value):
	if option == 'user/repo':
		try:
			response = requests.get('https://api.github.com/repos/' + value + '/contents/')
		except Exception as e:
			print(str(e), file=sys.stderr)
			sys.exit(1)
		if response.status_code == 200:
			string = '{ "repo": "'+ str(value) +'", "contents" : {}}'
			obj = json.loads(string)
			obj['contents']['/'] = response.json()
			with open(os.environ[envvar], 'w') as f:
				json.dump(obj, f)
				f.close()
			sys.exit()
		else:
			print(str(response.status_code) + ": " + response.reason, file=sys.stderr)
			print("Fs_Info_Message " + str(response.status_code) + ": " + response.reason)
			print('Fs_Request_Options')
			print('user/repo')
	sys.exit(1)

def vfs_list(path):
	try:
		with open(os.environ[envvar]) as f:
			try:
				obj = json.loads(f.read())
				f.close()
			except:
				sys.exit(1)
	except FileNotFoundError:
		sys.exit(1)
	if path not in obj['contents']:
		obj = get_dir_contents(obj, path)
	for element in obj['contents'][path]:
		if 'attrstr' not in element:
			if element['type'] == 'dir':
				element['attrstr'] = 'dr-xr-xr-x'
			elif element['type'] == 'file':
				element['attrstr'] = '-r--r--r--'
			elif element['type'] == 'symlink':
				element['attrstr'] = 'lr--r--r--'
		print(element['attrstr'] + '\t0000-00-00 00:00:00 \t' + str(element['size']) + '\t' + element['name'])
	try:
		with open(os.environ[envvar], 'w') as f:
			json.dump(obj, f)
			f.close()
	except FileNotFoundError:
		pass
	sys.exit()

def vfs_getfile(src, dst):
	try:
		with open(os.environ[envvar]) as f:
			obj = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	dirname = os.path.dirname(src)
	if dirname not in obj['contents']:
		obj = get_dir_contents(obj, dirname)
		with open(os.environ[envvar], 'w') as f:
			json.dump(obj, f)
			f.close()
	for element in obj['contents'][dirname]:
		if element['name'] == os.path.basename(src):
			with open(dst, 'wb') as f:
				try:
					response = requests.get(element['download_url'], stream=True)
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
							percent = int(downloaded * 100 / element['size'])
							print(percent)
							sys.stdout.flush()
				else:
					print(str(response.status_code) + ": " + response.reason, file=sys.stderr)

				f.close()
				sys.exit()
	sys.exit(1)

def vfs_properties(filename):
	try:
		with open(os.environ[envvar]) as f:
			obj = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in obj['contents'][os.path.dirname(filename)]:
		if element['name'] == os.path.basename(filename):
			print('SHA\t' + element['sha'])
	sys.exit()

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
elif verb == 'properties':
	vfs_properties(sys.argv[2])
elif verb == 'deinit':
	vfs_deinit()

sys.exit(1)
