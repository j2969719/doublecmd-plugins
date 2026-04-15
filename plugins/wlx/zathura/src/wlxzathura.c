#define _GNU_SOURCE
#include <gtk/gtk.h>
#ifdef GTK3PLUG
#include <gtk/gtkx.h>
#endif
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

gchar *command = NULL;
char plug_path[PATH_MAX];
gboolean is_plug_init = FALSE;

static void on_plug_added(GtkWidget *widget, gpointer data)
{
	gtk_spinner_stop(GTK_SPINNER(data));
	gtk_widget_hide(GTK_WIDGET(data));
	gtk_widget_show(widget);
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
#ifndef GTK3PLUG
	GtkWidget *main_box = gtk_vbox_new(FALSE, 5);
#else
	GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#endif
	gtk_widget_set_no_show_all(main_box, TRUE);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), main_box);
	GtkWidget *socket = gtk_socket_new();
	GtkWidget *spinner = gtk_spinner_new();
	gtk_widget_show(spinner);
	gtk_spinner_start(GTK_SPINNER(spinner));
	gtk_box_pack_start(GTK_BOX(main_box), socket, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(main_box), spinner, TRUE, FALSE, 0);
	g_signal_connect(socket, "plug-added", G_CALLBACK(on_plug_added), (gpointer)spinner);

	gchar **envp = g_environ_setenv(g_get_environ(), "GDK_CORE_DEVICE_EVENTS", "1", TRUE);
	envp = g_environ_setenv(envp, "FILE", FileToLoad, TRUE);
	envp = g_environ_setenv(envp, "PLUGPATH", plug_path, TRUE);
	gchar *string = g_strdup_printf("%ld", (gsize)gtk_socket_get_id(GTK_SOCKET(socket)));
	envp = g_environ_setenv(envp, "XID", string, TRUE);
	g_free(string);
	char *argv[] = {"sh", "-c", command, NULL};
	gboolean is_launched = g_spawn_async(NULL, argv, envp, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
	g_strfreev(envp);

	if (!is_launched)
	{
		gtk_widget_destroy(main_box);
		return NULL;
	}

	const gchar *role = gtk_window_get_role(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))));

	if (g_strcmp0(role, "TfrmViewer") != 0)
#ifndef GTK3PLUG
		gtk_widget_set_state(socket, GTK_STATE_INSENSITIVE);
#else
		gtk_widget_set_sensitive(socket, FALSE);
#endif

	gtk_widget_show(main_box);

	return main_box;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	if (Command == lc_copy)
	{
		gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text, -1);
		g_free(text);
		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}

static void wlxplug_atexit(void)
{
	g_free(command);
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	if (is_plug_init)
		return;

	Dl_info dlinfo;
	const char* cfg_file = "settings.ini";
	atexit(wlxplug_atexit);

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(&is_plug_init, &dlinfo) != 0)
	{
		g_strlcpy(plug_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plug_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);


		GError *err = NULL;
		gchar *params = NULL;
		gboolean is_extcfg = FALSE;

		GKeyFile *cfg = g_key_file_new();

		if (!g_key_file_load_from_file(cfg, plug_path, G_KEY_FILE_KEEP_COMMENTS, &err))
		{
			g_print("%s (%s): %s\n", PLUGNAME, plug_path, (err)->message);
			g_error_free(err);
		}
		else
		{
			params = g_key_file_get_string(cfg, "Settings", "Params", NULL);
			is_extcfg = g_key_file_get_boolean(cfg, "Settings", "ExternalCfg", NULL);
		}

		g_key_file_free(cfg);

		if (pos)
			*pos = '\0';


		if (is_extcfg)
			pos = "zathura --reparent=$XID \"$FILE\" %s --config-dir=\"$PLUGPATH\" --data-dir=\"$PLUGPATH\"";
		else
			pos = "zathura --reparent=$XID \"$FILE\" %s";

		command = g_strdup_printf(pos, params ? params : "");
		g_free(params);
	}

	is_plug_init = TRUE;
}
