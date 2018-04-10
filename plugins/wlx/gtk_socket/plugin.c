#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <dlfcn.h>
#include <string.h>
#include "wlxplugin.h"

#define _inifile "settings.ini"
#define _plgname "gtk_socket.wlx"
#define _frmtsrt "%d \"%s\""

static gchar *getfrmtstr(GKeyFile *cfg, gchar *ext, gchar *workdir)
{
	gchar *result;
	result = g_key_file_get_string(cfg, ext, "script", NULL);

	if (!result)
	{
		result = g_key_file_get_string(cfg, ext, "command", NULL);

		if ((!g_strrstr(result, "%s")) || (!g_strrstr(result, "%d")))
			return NULL;
	}
	else
	{
		if (g_file_test(result, G_FILE_TEST_EXISTS))
			result = g_strdup_printf("%s %s", result, _frmtsrt);
		else
		{
			result = g_strdup_printf("%s/%s", workdir, result);

			if (g_file_test(result, G_FILE_TEST_EXISTS))
				result = g_strdup_printf("%s %s", result, _frmtsrt);
			else
				return NULL;
		}
	}

	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	Dl_info dlinfo;
	GKeyFile *cfg;
	GError *err = NULL;
	GtkWidget *gFix;
	GtkWidget *socket;
	gchar *_cfgpath = "";
	gchar *_workdir;
	gchar *_command;
	gchar *_tmpsval;
	gchar *_fileext;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(_cfgpath, &dlinfo) != 0)
	{
		_workdir = g_path_get_dirname(dlinfo.dli_fname);
		_cfgpath = g_strdup_printf("%s/%s", _workdir, _inifile);
	}

	_fileext = g_strrstr(FileToLoad, ".");
	_fileext = g_strdup_printf("%s", _fileext + 1);
	_fileext = g_ascii_strdown(_fileext, -1);

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, _cfgpath, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("%s (%s): %s\n", _plgname, _cfgpath, (err)->message);
		return NULL;
	}
	else
	{
		_tmpsval = g_key_file_get_string(cfg, _fileext, "redirect", NULL);

		if (_tmpsval)
			_tmpsval = getfrmtstr(cfg, _tmpsval, _workdir);

		else
			_tmpsval = getfrmtstr(cfg, _fileext, _workdir);

	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);

	if (!_tmpsval)
		return NULL;

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	socket = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gFix), socket);

	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(socket));
	_command = g_strdup_printf(_tmpsval, id, FileToLoad);
	g_print("%s\n", _command);

	if (!g_spawn_command_line_async(_command, NULL))
	{
		gtk_widget_destroy(gFix);
		return NULL;
	}

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
	GKeyFile *cfg;
	GError *err = NULL;
	gsize extcount;
	gchar *detectstr = "";
	gchar *_cfgpath = "";
	gchar *_workdir;
	gchar **ext;
	guint index;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(_cfgpath, &dlinfo) != 0)
	{
		_workdir = g_path_get_dirname(dlinfo.dli_fname);
		_cfgpath = g_strdup_printf("%s/%s", _workdir, _inifile);
	}

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, _cfgpath, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("%s (%s): %s\n", _plgname, _cfgpath, (err)->message);

	}
	else
	{
		ext = g_key_file_get_groups(cfg, &extcount);

		for (index = 0; index < extcount; index++)
		{
			if (g_strcmp0(detectstr, "") != 0)
				detectstr = g_strdup_printf("|%s", detectstr);

			detectstr = g_strdup_printf("(EXT=\"%s\")%s", g_ascii_strup(ext[index],  -1), detectstr);
		}

		g_strlcpy(DetectString, detectstr, maxlen);
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{

	if (Command == lc_copy)
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
		                       gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY)), -1);

	return LISTPLUGIN_OK;
}
