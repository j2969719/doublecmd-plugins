#!/bin/env python3

import os
import sys
import json
import tempfile
import requests
from requests.exceptions import HTTPError

verb = sys.argv[1]

def get_jsonobj():
	try:
		with open(os.environ['DC_WFX_SCRIPT_JSON']) as f:
			try:
				obj = json.loads(f.read())
				f.close()
			except:
				print('Failed to get json object', file=sys.stderr)
				sys.exit(1)
	except FileNotFoundError:
		print('Failed to open json file', file=sys.stderr)
		sys.exit(1)
	return obj

def save_jsonobj(obj):
	try:
		with open(os.environ['DC_WFX_SCRIPT_JSON'], 'w') as f:
			json.dump(obj, f)
			f.close()
	except FileNotFoundError:
		pass

def get_dir_contents(obj, path):
	try:
		response = requests.get('https://api.github.com/repos/' + obj['repo'] + '/contents' + path)
	except Exception as e:
		print(str(e), file=sys.stderr)
		return None
	if response.status_code == 200:
		obj['contents'][path] = response.json()
		return obj
	else:
		print(str(response.status_code) + ": " + response.reason, file=sys.stderr)
		return None

def get_repo_root(value):
	string = '{ "repo": "'+ str(value) +'", "contents" : {}}'
	obj = json.loads(string)
	obj = get_dir_contents(obj, '/')
	if not obj:
		print("Fs_Info_Message WFX_SCRIPT_STR_ERR_JSON")
		sys.exit(1)
	else:
		save_jsonobj(obj)
		sys.exit()

def get_repo(value):
	try:
		response = requests.get('https://api.github.com/search/repositories', params = { 'q': value })
	except Exception as e:
		print("Fs_Info_Message " + str(e))
		sys.exit(1)
	if response.status_code == 200:
		res = response.json()
		choice = "Fs_MultiChoice WFX_SCRIPT_STR_REPO"
		for item in res['items']:
			choice = choice + '\t' + item['full_name']
		if choice != "Fs_MultiChoice WFX_SCRIPT_STR_REPO":
			print(choice)
		else:
			print('Fs_Info_Message \"' + value + '" WFX_SCRIPT_STR_ERR_NOTFOUND')
			print('Fs_Request_Options')
			print('WFX_SCRIPT_STR_SEARCH')
	else:
		print(str(response.status_code) + ": " + response.reason, file=sys.stderr)
		sys.exit(1)

def vfs_init():
	tf = tempfile.NamedTemporaryFile(suffix='_github.json', delete=False)
	filename = tf.name
	tf.close()
	print('Fs_Set_DC_WFX_SCRIPT_JSON ' + filename)
	print('Fs_CONNECT_Needed')
	print('Fs_PushValue WFX_SCRIPT_STR_REPO\tdoublecmd/doublecmd')
	print('Fs_YesNo_Message WFX_SCRIPT_STR_ASKSEARCH')
	sys.exit()

def vfs_setopt(option, value):
	if option == 'WFX_SCRIPT_STR_ASKSEARCH':
		print('Fs_Request_Options')
		if value == "Yes":
			print('WFX_SCRIPT_STR_SEARCH')
		else:
			print('WFX_SCRIPT_STR_REPO')
	elif option == 'WFX_SCRIPT_STR_SEARCH':
		get_repo(value)
	elif option == 'WFX_SCRIPT_STR_REPO':
		get_repo_root(value)
	elif option == 'WFX_SCRIPT_STR_NEWSEARCH':
		print('Fs_Request_Options')
		print('WFX_SCRIPT_STR_SEARCH')
	elif option == 'WFX_SCRIPT_STR_CHANGEREPO':
		print('Fs_Request_Options')
		print('WFX_SCRIPT_STR_REPO')
	sys.exit(1)

def vfs_list(path):
	obj = get_jsonobj()
	if path not in obj['contents']:
		obj = get_dir_contents(obj, path)
		if not obj:
			sys.exit(1)
		save_jsonobj(obj)
	for element in obj['contents'][path]:
		if 'attrstr' not in element:
			if element['type'] == 'dir':
				element['attrstr'] = 'dr-xr-xr-x'
			elif element['type'] == 'file':
				element['attrstr'] = '-r--r--r--'
			elif element['type'] == 'symlink':
				element['attrstr'] = 'lr--r--r--'
		print(element['attrstr'] + '\t0000-00-00 00:00:00 \t' + str(element['size']) + '\t' + element['name'])
	sys.exit()

def vfs_getfile(src, dst):
	obj = get_jsonobj()
	dirname = os.path.dirname(src)
	if dirname not in obj['contents']:
		obj = get_dir_contents(obj, dirname)
		with open(os.environ['DC_WFX_SCRIPT_JSON'], 'w') as f:
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
	obj = get_jsonobj()
	for element in obj['contents'][os.path.dirname(filename)]:
		if element['name'] == os.path.basename(filename):
			print('SHA\t' + element['sha'])
			print('url\t' + element['html_url'])
	print('Fs_PropsActs WFX_SCRIPT_STR_CHANGEREPO\tWFX_SCRIPT_STR_NEWSEARCH')
	sys.exit()

def vfs_deinit():
	os.remove(os.environ['DC_WFX_SCRIPT_JSON'])
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
