#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring  "(EXT=\"PDF\")|(EXT=\"DJVU\")|(EXT=\"DJV\")|(EXT=\"PS\")|(EXT=\"CBR\")|(EXT=\"CBZ\")"


static void plug_added(GtkWidget *widget, gpointer data)
{
	gtk_spinner_stop(GTK_SPINNER(data));
	gtk_widget_hide(GTK_WIDGET(data));
	gtk_widget_show(widget);
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	Dl_info dlinfo;
	static char plg_path[PATH_MAX], cfg_path[PATH_MAX];
	GtkWidget *gFix;
	GtkWidget *wspin;
	GtkWidget *zathura;
	GKeyFile *cfg;
	GError *err = NULL;
	gchar *param, *command;
	gboolean bval;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		g_strlcpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(dlinfo.dli_fname, '/');

		if (pos)
			g_strlcpy(plg_path, dlinfo.dli_fname, pos - dlinfo.dli_fname + 1);

		pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, "settings.ini");
	}

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("zathura.wlx (%s): %s\n", cfg_path, (err)->message);
		param = " ";
		bval = FALSE;
	}
	else
	{
		param = g_key_file_get_string(cfg, "Settings", "Params", NULL);

		if (!param)
			param = " ";

		bval = g_key_file_get_boolean(cfg, "Settings", "ExternalCfg", &err);

		if (!bval && !err)
			bval = FALSE;
		else
			bval = TRUE;
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	wspin = gtk_spinner_new();
	gtk_spinner_start(GTK_SPINNER(wspin));
	gtk_box_pack_start(GTK_BOX(gFix), wspin, TRUE, FALSE, 5);
	gtk_widget_show(wspin);

	zathura = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gFix), zathura);
	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(zathura));

	if (bval)
		command = g_strdup_printf("zathura --reparent=%d \"%s\" %s --config-dir=\"%s\" --data-dir=\"%s\"", id, FileToLoad, param, plg_path, plg_path);
	else
		command = g_strdup_printf("zathura --reparent=%d \"%s\" %s", id, FileToLoad, param);

	if (!g_spawn_command_line_async(command, NULL))
	{
		g_free(command);
		gtk_widget_destroy(gFix);
		return NULL;
	}

	g_free(command);
	g_signal_connect(zathura, "plug-added", G_CALLBACK(plug_added), (gpointer)wspin);

	gtk_widget_show(gFix);

	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen);
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
