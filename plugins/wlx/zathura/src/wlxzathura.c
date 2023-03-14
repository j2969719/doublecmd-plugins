#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring  "(EXT=\"PDF\")|(EXT=\"DJVU\")|(EXT=\"DJV\")|(EXT=\"PS\")|(EXT=\"CBR\")|(EXT=\"CBZ\")"

static char cfg_path[PATH_MAX];
static char plug_path[PATH_MAX];
const char* cfg_file = "settings.ini";

static void plug_added(GtkWidget *widget, gpointer data)
{
	gtk_spinner_stop(GTK_SPINNER(data));
	gtk_widget_hide(GTK_WIDGET(data));
	gtk_widget_show(widget);
}

static gchar* get_command_string(GdkNativeWindow id, char* FileToLoad)
{
	GKeyFile *cfg;
	GError *err = NULL;
	gchar *params, *command;
	gboolean extcfg;

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("zathura.wlx (%s): %s\n", cfg_path, (err)->message);
		params = " ";
		extcfg = FALSE;
	}
	else
	{
		params = g_key_file_get_string(cfg, "Settings", "Params", NULL);

		if (!params)
			params = " ";

		extcfg = g_key_file_get_boolean(cfg, "Settings", "ExternalCfg", NULL);
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);

	if (extcfg)
		command = g_strdup_printf("zathura --reparent=%d %s %s --config-dir=%s --data-dir=%s",
		                          id, g_shell_quote(FileToLoad), params, g_shell_quote(plug_path), g_shell_quote(plug_path));
	else
		command = g_strdup_printf("zathura --reparent=%d %s %s", id, g_shell_quote(FileToLoad), params);

	return command;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *wspin;
	GtkWidget *zathura;
	gchar **argv, **envp;

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	wspin = gtk_spinner_new();
	gtk_spinner_start(GTK_SPINNER(wspin));
	gtk_box_pack_start(GTK_BOX(gFix), wspin, TRUE, FALSE, 5);
	gtk_widget_show(wspin);

	zathura = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gFix), zathura);
	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(zathura));

	g_shell_parse_argv(get_command_string(id, FileToLoad), NULL, &argv, NULL);

	if (!argv)
	{
		gtk_widget_destroy(gFix);
		return NULL;
	}

	envp = g_environ_setenv(g_get_environ(), "GDK_CORE_DEVICE_EVENTS", "1", TRUE);

	if (!g_spawn_async(NULL, argv, envp, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL))
	{
		g_strfreev(argv);

		if (envp)
			g_strfreev(envp);

		gtk_widget_destroy(gFix);
		return NULL;
	}

	g_strfreev(argv);

	if (envp)
		g_strfreev(envp);

	g_signal_connect(zathura, "plug-added", G_CALLBACK(plug_added), (gpointer)wspin);

	const gchar *role = gtk_window_get_role(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))));

	if (g_strcmp0(role, "TfrmViewer") != 0)
		gtk_widget_set_state(zathura, GTK_STATE_INSENSITIVE);

	gtk_widget_show(gFix);

	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen - 1);
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
	else
		return LISTPLUGIN_ERROR;

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
