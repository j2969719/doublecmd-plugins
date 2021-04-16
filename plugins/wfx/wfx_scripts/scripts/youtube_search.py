#!/bin/env python

# pip install youtube-search-python
# pip install pytube

import os
import sys
import json
import tempfile
from pytube import YouTube, helpers
from youtubesearchpython import VideosSearch

verb = sys.argv[1]
envvar = 'DC_WFX_SCRIPT_DATA'
video_res = '360p'
video_fps = 60

def print_progress(stream = None, chunk = None, remaining = None):
	current = (stream.filesize - remaining)/stream.filesize
	percent = ('{0:.0f}').format(current*100)
	print(percent)

def vfs_init():
	# print('Fs_GetSupportedField_Needed') # nah, its slow anyway
	tf = tempfile.NamedTemporaryFile(suffix='_youtube.json', delete=False)
	filename = tf.name
	tf.close()
	print('Fs_Set_'+ envvar +' ' + filename)
	print('Fs_Request_Options')
	print('Search video')
	sys.exit()

def vfs_setopt(option, value):
	videosSearch = VideosSearch(value)
	res_array = videosSearch.result()['result']
	with open(os.environ[envvar], 'w') as f:
		json.dump(res_array, f)
		f.close()
	sys.exit()

def vfs_list(path):
	try:
		with open(os.environ[envvar]) as f:
			res_array = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in res_array:
		if 'filename' not in element:
			element['filename'] = helpers.safe_filename(element['title']) + '.mp4'
		print('-r--r--r--\t0000-00-00 00:00:00 \t404\t' + element['filename'])
	try:
		with open(os.environ[envvar], 'w') as f:
			json.dump(res_array, f)
			f.close()
	except FileNotFoundError:
		pass
	sys.exit()

def vfs_getfile(src, dst):
	try:
		with open(os.environ[envvar]) as f:
			res_array = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in res_array:
		if element['filename'] == src[1:]:
			yt = YouTube(element['link'], on_progress_callback=print_progress)
			stream = yt.streams.filter(fps=video_fps, res=video_res, mime_type='video/mp4').first()
			if stream is None:
				stream = yt.streams.first()
			stream.download(output_path=os.path.dirname(dst))
			sys.exit()
	sys.exit(1)

def vfs_execute(filename):
	try:
		with open(os.environ[envvar]) as f:
			res_array = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in res_array:
		if element['filename'] == filename[1:]:
			os.system('xdg-open ' + element['link'])
			sys.exit()
	sys.exit(1)

def vfs_properties(filename):
	try:
		with open(os.environ[envvar]) as f:
			res_array = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in res_array:
		if element['filename'] == filename[1:]:
			if 'title' in element and not element['title'] is None:
				print('Title\t' + element['title'])
			if 'duration' in element and not element['duration'] is None:
				print('Duration\t' + element['duration'])
			if 'publishedTime' in element and not element['publishedTime'] is None:
				print('Published Time\t' + element['publishedTime'])
			if 'link' in element and not element['link'] is None:
				print('Link\t' + element['link'])
			if 'viewCount' in element and not element['viewCount'] is None:
				if 'viewCount' in element and not element['viewCount']['short'] is None:
					print('Views\t' + element['viewCount']['short'])
	sys.exit()

def vfs_getfields():
	print('title')
	print('duration')
	print('publishedTime')
	print('link')
	print('type')
	sys.exit()

def vfs_getvalue(field, filename):
	if os.environ[envvar] is None:
		sys.exit(1)
	try:
		with open(os.environ[envvar]) as f:
			res_array = json.loads(f.read())
			f.close()
	except FileNotFoundError:
		sys.exit(1)
	for element in res_array:
		if element['filename'] == filename[1:]:
			if 'filename' in element:
				print(element[field])
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
elif verb == 'run':
	vfs_execute(sys.argv[2])
elif verb == 'properties':
	vfs_properties(sys.argv[2])
elif verb == 'getfields':
	vfs_getfields()
elif verb == 'getvalue':
	vfs_getvalue(sys.argv[2], sys.argv[3])
elif verb == 'deinit':
	vfs_deinit()

sys.exit(1)