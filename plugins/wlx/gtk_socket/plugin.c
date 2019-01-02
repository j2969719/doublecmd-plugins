#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <dlfcn.h>
#include <glib.h>
#include <magic.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "EXT=\"*\""
#define _plgname "gtk_socket.wlx"
#define _frmtsrt "%d \"%s\""

static char cfg_path[PATH_MAX];
static char plug_path[PATH_MAX];
const char* cfg_file = "settings.ini";

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

gchar *get_mime_type(const gchar *Filename)
{
	magic_t magic_cookie;
	gchar *result;

	magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);

	if (magic_load(magic_cookie, NULL) != 0)
	{
		magic_close(magic_cookie);
		return NULL;
	}

	result = g_strdup(magic_file(magic_cookie, Filename));
	magic_close(magic_cookie);
	return result;
}

static gchar *cfg_get_frmt_str(GKeyFile *Cfg, const gchar *Group)
{
	gchar *result, *cfg_value, *tmp;
	cfg_value = g_key_file_get_string(Cfg, Group, "script", NULL);

	if (!cfg_value)
	{
		result = g_key_file_get_string(Cfg, Group, "command", NULL);

		if ((!result) || (!g_strrstr(result, "%s")) || (!g_strrstr(result, "%d")))
			result = NULL;
	}
	else
	{
		tmp = g_strdup_printf("%s/%s", plug_path, cfg_value);

		if (g_file_test(tmp, G_FILE_TEST_EXISTS))
			result = g_strdup_printf("%s %s", g_shell_quote(tmp), _frmtsrt);
		else if (g_file_test(cfg_value, G_FILE_TEST_EXISTS))
			result = g_strdup_printf("%s %s", cfg_value, _frmtsrt);
		else
			result = NULL;
	}

	return result;
}

gchar *cfg_find_value(GKeyFile *Cfg, const gchar *Group)
{
	gchar *result, *redirect;
	redirect = g_key_file_get_string(Cfg, Group, "redirect", NULL);

	if (redirect)
		result = cfg_get_frmt_str(Cfg, redirect);
	else
		result = cfg_get_frmt_str(Cfg, Group);
	g_free(redirect);
	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GKeyFile *cfg;
	GError *err = NULL;
	GtkWidget *gFix;
	GtkWidget *socket;
	gchar *file_ext, *mime_type;
	gchar *command;
	gchar *frmt_str = NULL;

	mime_type = get_mime_type(FileToLoad);
	file_ext = get_file_ext(FileToLoad);

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("%s (%s): %s\n", _plgname, cfg_path, (err)->message);
	else
	{
		if (file_ext)
			frmt_str = cfg_find_value(cfg, file_ext);
		if (mime_type && (!file_ext || !frmt_str))
			frmt_str = cfg_find_value(cfg, mime_type);
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);

	if (!frmt_str)
		return NULL;

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	socket = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gFix), socket);

	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(socket));
	command = g_strdup_printf(frmt_str, id, FileToLoad);
	g_print("%s\n", command);

	if (!g_spawn_command_line_async(command, NULL))
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
	g_strlcpy(DetectString, _detectstring, maxlen-1);
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

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		strncpy(plug_path, g_path_get_dirname(cfg_path), PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}
}
