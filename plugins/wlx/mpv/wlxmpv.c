#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring  "(EXT=\"AVI\")|(EXT=\"MKV\")|(EXT=\"FLV\")|(EXT=\"MPG\")|\
(EXT=\"MPEG\")|(EXT=\"MP4\")|(EXT=\"3GP\")|(EXT=\"MP3\")|(EXT=\"OGG\")|(EXT=\"WMA\")|\
(EXT=\"BIK\")|(EXT=\"VOC\")|(EXT=\"WAV\")|(EXT=\"WEBM\")|(EXT=\"VOB\")|(EXT=\"ROQ\")|\
(EXT=\"IVF\")|(EXT=\"MOV\")|(EXT=\"FLAC\")|(EXT=\"WMV\")"
#define _defaultparams "--force-window=yes --keep-open=yes --script-opts=osc-visibility=always\
 --cursor-autohide-fs-only"
#define _defaultcmd "mpv"
#define _configfile "settings.ini"

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	Dl_info dlinfo;
	static char cfg_path[PATH_MAX];
	GKeyFile *cfg;
	GError *err = NULL;
	gboolean is_certain = FALSE;
	gboolean bval = FALSE;
	gchar *_cmd, *_params;
	gchar *_mediatype;
	GdkNativeWindow id;
	GtkWidget *gFix;
	GtkWidget *mpv;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, _configfile);
	}

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("mpv.wlx (%s): %s\n", cfg_path, (err)->message);
		_cmd = _defaultcmd;
		_params = _defaultparams;
	}
	else
	{
		gchar *ext = g_strrstr(FileToLoad, ".");
		ext = g_strdup_printf("%s", ext + 1);
		ext = g_ascii_strdown(ext, -1);
		gchar *content_type = g_content_type_guess(FileToLoad, NULL, 0, &is_certain);

		bval = g_key_file_get_boolean(cfg, "Default", "PrintContentType", NULL);

		if (bval)
			g_print("content_type = %s\n", content_type);

		bval = g_key_file_get_boolean(cfg, "Default", "GTK_Socket", NULL);
		_mediatype = g_strdup_printf("%.5s", content_type);

		_cmd = g_key_file_get_string(cfg, ext, "Cmd", NULL);

		if (!_cmd)
		{
			_cmd = g_key_file_get_string(cfg, content_type, "Cmd", NULL);

			if (!_cmd)
			{
				_cmd = g_key_file_get_string(cfg, _mediatype, "Cmd", NULL);

				if (!_cmd)
					_cmd = g_key_file_get_string(cfg, "Default", "Cmd", NULL);

				if (!_cmd)
					_cmd = _defaultcmd;
			}
		}

		_params = g_key_file_get_string(cfg, ext, "Params", NULL);

		if (!_params)
		{
			_params = g_key_file_get_string(cfg, content_type, "Params", NULL);

			if (!_params)
			{
				_params = g_key_file_get_string(cfg, _mediatype, "Params", NULL);

				if (!_params)
					_params = g_key_file_get_string(cfg, "Default", "Params", NULL);

				if (!_params)
					_params = _defaultparams;
			}
		}

		g_free(content_type);
		g_free(_mediatype);
		g_free(ext);
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);


	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);

	if (bval)
	{
		mpv = gtk_socket_new();
		gtk_container_add(GTK_CONTAINER(gFix), mpv);
		id = gtk_socket_get_id(GTK_SOCKET(mpv));
	}
	else
	{
		mpv = gtk_drawing_area_new();
		gtk_container_add(GTK_CONTAINER(gFix), mpv);
		gtk_widget_realize(mpv);
		id = GDK_WINDOW_XID(gtk_widget_get_window(mpv));
	}


	gchar *command = g_strdup_printf("%s %s --wid=%d \"%s\"", _cmd, _params, id, FileToLoad);

	if ((id == 0) || (!g_spawn_command_line_async(command, NULL)))
	{
		g_free(command);
		gtk_widget_destroy(gFix);
		return NULL;
	}

	g_free(command);

	gtk_widget_show_all(gFix);

	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	Dl_info dlinfo;
	static char cfg_path[PATH_MAX];
	GKeyFile *cfg;
	gchar *_detectstr;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, _configfile);
	}

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL))
		g_strlcpy(DetectString, _detectstring, maxlen);
	else
	{
		_detectstr = g_key_file_get_string(cfg, "Default", "DetectString", NULL);

		if (!_detectstr)
			g_strlcpy(DetectString, _detectstring, maxlen);
		else
			g_strlcpy(DetectString, _detectstr, maxlen);
	}

	g_key_file_free(cfg);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	return LISTPLUGIN_ERROR;
}
