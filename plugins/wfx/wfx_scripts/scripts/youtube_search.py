#!/bin/env python3

# slow af

# pip install youtube-search-python
# pip install pytube

import os
import sys
import json
import tempfile
from pytube import YouTube, helpers
from youtubesearchpython import VideosSearch

verb = sys.argv[1]
videostr = 'WFX_SCRIPT_STR_1080P\tWFX_SCRIPT_STR_720P\tWFX_SCRIPT_STR_480P\tWFX_SCRIPT_STR_360P\tWFX_SCRIPT_STR_240P'
audiostr = 'WFX_SCRIPT_STR_128ABR\tWFX_SCRIPT_STR_48ABR\tWFX_SCRIPT_STR_32ABR'
modestr = 'WFX_SCRIPT_STR_MODE\t' + videostr + '\t' + audiostr


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

def get_env_text(var):
	try:
		return os.environ[var]
	except:
		return var

def print_progress(stream = None, chunk = None, remaining = None):
	current = (stream.filesize - remaining)/stream.filesize
	percent = ('{0:.0f}').format(current*100)
	print(percent)
	sys.stdout.flush()

def prepare_filename(res_array, title):
	extra = ''
	index = 1
	while True:
		is_found = False
		candidate = helpers.safe_filename(title) + extra
		for element in res_array:
			if 'filename' in element and element['filename'] == candidate:
				is_found = True
		if is_found == True:
			extra = ' (' + str(index) + ')'
			index += 1
		else:
			break
	return candidate + '.' + os.environ['DC_WFX_SCRIPT_EXT']

def vfs_init():
	tf = tempfile.NamedTemporaryFile(suffix='_youtube.json', delete=False)
	filename = tf.name
	tf.close()
	print('Fs_Set_DC_WFX_SCRIPT_JSON ' + filename)
	print('Fs_CONNECT_Needed')
	print('Fs_GetValues_Needed')
	print('Fs_Request_Options')
	print('Fs_Set_DC_WFX_SCRIPT_EXT mp4')
	print('Fs_Set_DC_WFX_SCRIPT_RES 360p')
	print('Fs_Set_DC_WFX_SCRIPT_ABR 128kbps')
	print('Fs_PushValue WFX_SCRIPT_STR_MODE\tWFX_SCRIPT_STR_360P')
	print('Fs_MultiChoice ' + modestr)
	print('WFX_SCRIPT_STR_SEARCH')
	sys.exit()

def vfs_setopt(option, value):
	if option == 'WFX_SCRIPT_STR_SEARCH':
		videosSearch = VideosSearch(value)
		res_array = videosSearch.result()['result']
		save_jsonobj(res_array)
	elif option == 'WFX_SCRIPT_STR_ACTSEARCH':
		print('Fs_Request_Options\nWFX_SCRIPT_STR_SEARCH')
	elif option == 'WFX_SCRIPT_STR_ACTMODE':
		print('Fs_MultiChoice ' + modestr)
		print('Fs_Request_Options\nWFX_SCRIPT_STR_SEARCH')
	elif option == 'WFX_SCRIPT_STR_MODE':
		if value == 'WFX_SCRIPT_STR_1080P':
			print('Fs_Set_DC_WFX_SCRIPT_EXT mp4')
			print('Fs_Set_DC_WFX_SCRIPT_RES 1080p')
		if value == 'WFX_SCRIPT_STR_720P':
			print('Fs_Set_DC_WFX_SCRIPT_EXT mp4')
			print('Fs_Set_DC_WFX_SCRIPT_RES 720p')
		if value == 'WFX_SCRIPT_STR_480P':
			print('Fs_Set_DC_WFX_SCRIPT_EXT mp4')
			print('Fs_Set_DC_WFX_SCRIPT_RES 480p')
		if value == 'WFX_SCRIPT_STR_360P':
			print('Fs_Set_DC_WFX_SCRIPT_EXT mp4')
			print('Fs_Set_DC_WFX_SCRIPT_RES 360p')
		if value == 'WFX_SCRIPT_STR_240P':
			print('Fs_Set_DC_WFX_SCRIPT_EXT mp4')
			print('Fs_Set_DC_WFX_SCRIPT_RES 240p')
		if value == 'WFX_SCRIPT_STR_128ABR':
			print('Fs_Set_DC_WFX_SCRIPT_EXT m4a')
			print('Fs_Set_DC_WFX_SCRIPT_ABR 128kbps')
		if value == 'WFX_SCRIPT_STR_48ABR':
			print('Fs_Set_DC_WFX_SCRIPT_EXT m4a')
			print('Fs_Set_DC_WFX_SCRIPT_ABR 48kbps')
		if value == 'WFX_SCRIPT_STR_32ABR':
			print('Fs_Set_DC_WFX_SCRIPT_EXT m4a')
			print('Fs_Set_DC_WFX_SCRIPT_ABR 32kbps')
	sys.exit()

def vfs_list(path):
	res_array = get_jsonobj()
	for element in res_array:
		if 'filename' not in element:
			element['filename'] = prepare_filename(res_array, element['title'])
		if not element['duration'] is None:
			print('-r--r--r--\t0000-00-00 00:00:00 \t-\t' + element['filename'])
	save_jsonobj(res_array)
	sys.exit()

def vfs_getfile(src, dst):
	res_array = get_jsonobj()
	for element in res_array:
		if element['filename'] == src[1:]:
			yt = YouTube(element['link'], on_progress_callback=print_progress)
			if src[-3:] == 'm4a':
				stream = yt.streams.filter(abr=os.environ['DC_WFX_SCRIPT_ABR'], mime_type='audio/mp4').first()
			else:
				stream = yt.streams.filter(res=os.environ['DC_WFX_SCRIPT_RES'], mime_type='video/mp4').first()
			if stream is None:
				stream = yt.streams.first()
				print(get_env_text('ENV_WFX_SCRIPT_STR_ERRSTREAM') + ': ' + str(stream), file=sys.stderr)
			stream.download(output_path=os.path.dirname(dst), filename=os.path.basename(dst))
			sys.exit()
	sys.exit(1)

def vfs_execute(filename):
	res_array = get_jsonobj()
	for element in res_array:
		if element['filename'] == filename[1:]:
			print('Fs_Open ' + element['link'])
			sys.exit()
	sys.exit(1)

def vfs_properties(filename):
	res_array = get_jsonobj()
	for element in res_array:
		if element['filename'] == filename[1:]:
			if 'title' in element and not element['title'] is None:
				print('WFX_SCRIPT_STR_TITLE\t' + element['title'])
			if 'duration' in element and not element['duration'] is None:
				print('WFX_SCRIPT_STR_DURATION\t' + element['duration'])
			if 'publishedTime' in element and not element['publishedTime'] is None:
				print('WFX_SCRIPT_STR_PUBLTIME\t' + element['publishedTime'])
			if 'link' in element and not element['link'] is None:
				print('URL\t' + element['link'])
			if 'viewCount' in element and not element['viewCount'] is None:
				if 'viewCount' in element and not element['viewCount']['short'] is None:
					print('WFX_SCRIPT_STR_VIEWS\t' + element['viewCount']['short'])
	print('Fs_PropsActs WFX_SCRIPT_STR_ACTSEARCH\tWFX_SCRIPT_STR_ACTMODE')
	sys.exit()

def vfs_getvalues(filename):
	res_array = get_jsonobj()
	for element in res_array:
		print(element['filename'] + '\t' + element['duration'])
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
elif verb == 'run':
	vfs_execute(sys.argv[2])
elif verb == 'properties':
	vfs_properties(sys.argv[2])
elif verb == 'getfields':
	vfs_getfields()
elif verb == 'getvalues':
	vfs_getvalues(sys.argv[2])
elif verb == 'deinit':
	vfs_deinit()

sys.exit(1)
