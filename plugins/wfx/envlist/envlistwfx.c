#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include "wfxplugin.h"

#define _createstr "/<Create New>"

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
gint ienv;
gchar **env;
gchar *curenvname;
GtkWidget *entrname;


static void CloseDialog(GtkWidget *widget, gpointer data)
{
	gtk_grab_remove(GTK_WIDGET(data));
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void SetValue(GtkWidget *widget, gpointer data)
{
	const gchar *envval = gtk_entry_get_text(GTK_ENTRY(data));

	if (!g_setenv(curenvname, envval, TRUE))
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(data))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		                    "Could not set environment variable \"%s\" to \"%s\"!", curenvname, envval);
		gtk_window_set_title(GTK_WINDOW(dialog), "Environment Variable");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}

	gtk_grab_remove(gtk_widget_get_toplevel(GTK_WIDGET(data)));
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(data)));
}

static void SetNewEnv(GtkWidget *widget, gpointer data)
{
	const gchar *envname = gtk_entry_get_text(GTK_ENTRY(entrname));
	const gchar *envval = gtk_entry_get_text(GTK_ENTRY(data));

	if (!g_setenv(envname, envval, TRUE))
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(data))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		                    "Could not create new environment variable \"%s\"!", envname);
		gtk_window_set_title(GTK_WINDOW(dialog), "Environment Variable");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	else
	{
		gtk_grab_remove(gtk_widget_get_toplevel(GTK_WIDGET(data)));
		gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(data)));
	}
}

static void ShowEnv(gchar *name, const gchar *val)
{
	GtkWidget *dialog;
	GtkWidget *envlabel;
	GtkWidget *value;
	GtkWidget *button;


	gchar *envn = g_strdup_printf("%s:", name);

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Environment Variable");
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);

	envlabel = gtk_label_new(envn);
	gtk_misc_set_alignment(GTK_MISC(envlabel), 0, 1);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), envlabel, TRUE, TRUE, 0);
	gtk_widget_show(envlabel);

	value = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(value), val);
	gtk_widget_set_size_request(value, 550, -1);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), value, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(value), "activate", G_CALLBACK(SetValue), (gpointer)(GtkWidget*)(value));
	gtk_widget_show(value);

	button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(SetValue), (gpointer)(GtkWidget*)(value));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(CloseDialog), dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	gtk_widget_show(dialog);
	gtk_widget_grab_focus(button);
	g_free(envn);
}

static void NewEnv(void)
{
	GtkWidget *dialog;
	GtkWidget *envlabel;
	GtkWidget *value;
	GtkWidget *button;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Environment Variable");
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);

	envlabel = gtk_label_new("Environment variable name:");
	gtk_misc_set_alignment(GTK_MISC(envlabel), 0, 1);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), envlabel, TRUE, TRUE, 0);
	gtk_widget_show(envlabel);

	entrname = gtk_entry_new();
	value = gtk_entry_new();
	gtk_widget_set_size_request(entrname, 350, -1);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entrname, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(entrname), "activate", G_CALLBACK(SetNewEnv), (gpointer)(GtkWidget*)(value));
	gtk_widget_show(entrname);

	envlabel = gtk_label_new("Environment variable value:");
	gtk_misc_set_alignment(GTK_MISC(envlabel), 0, 1);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), envlabel, TRUE, TRUE, 0);
	gtk_widget_show(envlabel);

	gtk_widget_set_size_request(value, 350, -1);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), value, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(value), "activate", G_CALLBACK(SetNewEnv), (gpointer)(GtkWidget*)(value));
	gtk_widget_show(value);

	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(SetNewEnv), (gpointer)(GtkWidget*)(value));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(CloseDialog), dialog);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	gtk_widget_show(dialog);
}

void GetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;
	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
	ienv = 0;
	env = g_listenv();
	g_strlcpy(FindData->cFileName, _createstr + 1, PATH_MAX);
	GetCurrentFileTime(&FindData->ftLastWriteTime);
	return (HANDLE)(1985);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	if (env[ienv] != NULL)
	{
		g_strlcpy(FindData->cFileName, env[ienv], PATH_MAX);
		FindData->nFileSizeLow = strlen(g_getenv(env[ienv]));
		GetCurrentFileTime(&FindData->ftLastWriteTime);
		ienv++;
		return TRUE;
	}
	else
		return FALSE;
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	if (env)
		g_strfreev(env);

	return ienv;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	g_unsetenv(RemoteName + 1);
	return TRUE;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{

	if (g_strcmp0(RemoteName, _createstr) == 0)
		NewEnv();
	else
	{
		curenvname = g_strdup(RemoteName + 1);
		const gchar *envv = g_getenv(RemoteName + 1);
		ShowEnv(curenvname, envv);
	}

	return FS_EXEC_OK;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);

	if (err)
		return FS_FILE_USERABORT;

	if (g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	const gchar *envv = g_getenv(RemoteName + 1);

	if (!g_file_set_contents(LocalName, envv, -1, NULL))
		return FS_FILE_WRITEERROR;


	return FS_FILE_OK;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Environment variables", maxlen-1);
}
