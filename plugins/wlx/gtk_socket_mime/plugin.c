#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <dlfcn.h>
#include <glib.h>
#include <magic.h>
#include <string.h>
#include "wlxplugin.h"

#define _inifile "settings.ini"
#define _plgname "gtk_socket_mime.wlx"
#define _frmtsrt "%d \"%s\""

static gchar *getfrmtstr(GKeyFile *cfg, const gchar *ext, gchar *workdir)
{
	gchar *result;
	result = g_key_file_get_string(cfg, ext, "script", NULL);

	if (!result)
	{
		result = g_key_file_get_string(cfg, ext, "command", NULL);

		if ((!result) || (!g_strrstr(result, "%s")) || (!g_strrstr(result, "%d")))
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
	magic_t magic_cookie;
	GtkWidget *gFix;
	GtkWidget *socket;
	gchar *_cfgpath = "";
	gchar *_workdir;
	gchar *_command;
	gchar *_tmpsval;
	const gchar *_filemime;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(_cfgpath, &dlinfo) != 0)
	{
		_workdir = g_path_get_dirname(dlinfo.dli_fname);
		_cfgpath = g_strdup_printf("%s/%s", _workdir, _inifile);
	}

	magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);

	if (magic_load(magic_cookie, NULL) != 0)
	{
		magic_close(magic_cookie);
		return NULL;
	}

	_filemime = magic_file(magic_cookie, FileToLoad);

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, _cfgpath, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("%s (%s): %s\n", _plgname, _cfgpath, (err)->message);
		magic_close(magic_cookie);
		return NULL;
	}
	else
	{
		_tmpsval = g_key_file_get_string(cfg, _filemime, "redirect", NULL);

		if (_tmpsval)
			_tmpsval = getfrmtstr(cfg, _tmpsval, _workdir);

		else
			_tmpsval = getfrmtstr(cfg, _filemime, _workdir);

	}

	g_key_file_free(cfg);


	if (err)
		g_error_free(err);

	if (!_tmpsval)
	{
		magic_close(magic_cookie);
		return NULL;
	}

	magic_close(magic_cookie);

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
	g_strlcpy(DetectString, "EXT=\"*\"", maxlen);
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
