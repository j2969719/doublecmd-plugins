#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <dlfcn.h>
#include <limits.h>
#include <signal.h>
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

static char cfg_path[PATH_MAX];

gchar *get_file_ext(const gchar *Filename)
{
	if (g_file_test(Filename, G_FILE_TEST_IS_DIR))
		return NULL;

	gchar *basename, *result, *tmpval;

	basename = g_path_get_basename(Filename);
	result = g_strrstr(basename, ".");

	if (result)
	{
		if (g_strcmp0(result, basename) != 0)
		{
			tmpval = g_strdup_printf("%s", result + 1);
			result = g_ascii_strdown(tmpval, -1);
			g_free(tmpval);
		}
		else
			result = NULL;
	}

	g_free(basename);

	return result;
}

gchar *cfg_get_value(GKeyFile *Cfg, const gchar *Key, const gchar *FileExt, const gchar *ContenType)
{
	gchar *result = NULL, *mediatype;

	if (FileExt)
		result = g_key_file_get_string(Cfg, FileExt, Key, NULL);

	if (!result && ContenType)
	{
		result = g_key_file_get_string(Cfg, ContenType, Key, NULL);

		if (!result)
		{
			mediatype = g_strdup_printf("%.5s", ContenType);

			if (mediatype)
			{
				result = g_key_file_get_string(Cfg, mediatype, Key, NULL);
				g_free(mediatype);
			}
		}
	}

	if (!result)
		result = g_key_file_get_string(Cfg, "Default", Key, NULL);


	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GKeyFile *cfg;
	GError *err = NULL;
	gboolean is_certain = FALSE;
	gboolean bval = FALSE;
	gchar *cmdstr, *params;
	GdkNativeWindow id;
	GtkWidget *gFix;
	GtkWidget *mpv;
	GPid mpv_pid;
	gchar **argv;

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("mpv.wlx (%s): %s\n", cfg_path, (err)->message);
		cmdstr = _defaultcmd;
		params = _defaultparams;
	}
	else
	{
		gchar *ext = get_file_ext(FileToLoad);
		gchar *content_type = g_content_type_guess(FileToLoad, NULL, 0, &is_certain);

		bval = g_key_file_get_boolean(cfg, "Default", "PrintContentType", NULL);

		if (bval)
			g_print("content_type = %s\n", content_type);

		bval = g_key_file_get_boolean(cfg, "Default", "GTK_Socket", NULL);

		cmdstr = cfg_get_value(cfg, "Cmd", ext, content_type);

		if (!cmdstr)
			cmdstr = _defaultcmd;

		params = cfg_get_value(cfg, "Params", ext, content_type);

		if (!params)
			params = _defaultparams;
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

	gchar *command = g_strdup_printf("%s %s --wid=%d %s", cmdstr, params, id, g_shell_quote(FileToLoad));

	if (!g_shell_parse_argv(command, NULL, &argv, NULL))
	{
		g_free(command);
		gtk_widget_destroy(gFix);
		return NULL;
	}

	g_free(command);

	if ((id == 0) || (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &mpv_pid, NULL)))
	{
		gtk_widget_destroy(gFix);
		return NULL;
	}

	g_object_set_data(G_OBJECT(gFix), "pid", GINT_TO_POINTER(mpv_pid));

	gtk_widget_show_all(gFix);

	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	kill(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(ListWin), "pid")), SIGINT);
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	GKeyFile *cfg;
	gchar *_detectstr;

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL))
		g_strlcpy(DetectString, _detectstring, maxlen - 1);
	else
	{
		_detectstr = g_key_file_get_string(cfg, "Default", "DetectString", NULL);

		if (!_detectstr)
			g_strlcpy(DetectString, _detectstring, maxlen - 1);
		else
			g_strlcpy(DetectString, _detectstr, maxlen - 1);
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

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, _configfile);
	}
}
