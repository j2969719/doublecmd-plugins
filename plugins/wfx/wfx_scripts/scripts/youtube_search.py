#!/bin/env python

# pip install youtube-search-python
# pip install pytube

import os
import sys
import json
from pytube import YouTube, helpers
from youtubesearchpython import VideosSearch

verb = sys.argv[1]
tempfile = '/tmp/caramelldansen.json'
max_results = 10

def vfs_init():
	print("Search video:")
	sys.exit()

def vfs_setopt(option, value):
	videosSearch = VideosSearch(value, limit=max_results)
	res_array = videosSearch.result()['result']
	with open(tempfile, 'w') as f:
		json.dump(res_array, f)
		f.close()
	sys.exit()

def vfs_list(path):
	try:
		with open(tempfile) as f:
			res_array = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in res_array:
		if 'filename' not in element:
			element['filename'] = helpers.safe_filename(element['title']) + '.mp4'
		print('-r--r--r--\t0000-00-00 00:00:00 \t404\t' + element['filename'])
	try:
		with open(tempfile, 'w') as f:
			json.dump(res_array, f)
			f.close()
	except FileNotFoundError:
		pass
	sys.exit()

def vfs_getfile(src, dst):
	try:
		with open(tempfile) as f:
			res_array = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in res_array:
		if element['filename'] == src[1:]:
			yt = YouTube(element['link'])
			yt.streams.first().download(output_path=os.path.dirname(dst))
			sys.exit()
	sys.exit(1)

def vfs_execute(filename):
	try:
		with open(tempfile) as f:
			res_array = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in res_array:
		if element['filename'] == filename[1:]:
			os.system("xdg-open " + element['link'])
			sys.exit()
	sys.exit(1)

def vfs_getfields():
	print("title")
	print("duration")
	print("publishedTime")
	print("link")
	print("type")
	sys.exit()

def vfs_getvalue(field, filename):
	try:
		with open(tempfile) as f:
			res_array = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in res_array:
		if element['filename'] == filename[1:]:
			print(element[field])
			sys.exit()
	sys.exit(1)

def vfs_deinit():
	os.remove(tempfile)
	sys.exit()



if verb == "init":
	vfs_init()
elif verb == "setopt":
	vfs_setopt(sys.argv[2], sys.argv[3])
elif verb == "list":
	vfs_list(sys.argv[2])
elif verb == "copyout":
	vfs_getfile(sys.argv[2], sys.argv[3])
elif verb == "run":
	vfs_execute(sys.argv[2])
elif verb == "properties":
	vfs_init()
elif verb == "getfields":
	vfs_getfields()
elif verb == "getvalue":
	vfs_getvalue(sys.argv[2], sys.argv[3])
elif verb == "deinit":
	vfs_deinit()

sys.exit(1)
