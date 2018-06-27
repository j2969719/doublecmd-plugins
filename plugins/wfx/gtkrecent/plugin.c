#include <gtk/gtk.h>
#include <glib.h>
#include <sys/stat.h>
#include <string.h>
#include "extension.h"
#include "wfxplugin.h"

#define _plugname "GTKRecent"
#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
GtkRecentManager *manager;
GList *list;

gboolean UnixTimeToFileTime(unsigned long mtime, LPFILETIME ft)
{
	gint64 ll = Int32x32To64(mtime, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
	return TRUE;
}

static void ShowGError(gchar *str, GError *err)
{
	GtkWidget *dialog = gtk_message_dialog_new(NULL,
	                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s: %s", str, (err)->message);
	gtk_window_set_title(GTK_WINDOW(dialog), _plugname);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

gboolean SetFindData(WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
	struct stat buf;

	if (list != NULL)
	{
		gchar *fname = g_filename_from_uri(gtk_recent_info_get_uri(list->data), NULL, NULL);
		g_strlcpy(FindData->cFileName, fname, PATH_MAX);
		UnixTimeToFileTime(gtk_recent_info_get_visited(list->data), &FindData->ftLastWriteTime);

		if (stat(fname, &buf) != 0)
			FindData->nFileSizeLow = 0;
		else
			FindData->nFileSizeLow = buf.st_size;

		g_free(fname);
		gtk_recent_info_unref(list->data);
		list = list->next;
		return TRUE;
	}

	return FALSE;
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;

	manager = gtk_recent_manager_get_default();

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	list = gtk_recent_manager_get_items(manager);

	if (SetFindData(FindData))
		return (HANDLE)(1985);
	else
		return (HANDLE)(-1);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	if (SetFindData(FindData))
		return TRUE;
	else
		return FALSE;
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	if (list != NULL)
		g_list_free(list);

	return 0;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	gchar *uri = g_filename_to_uri(RemoteName + 1, NULL, NULL);
	GError *err = NULL;

	if (!gtk_recent_manager_remove_item(manager, uri, &err))
	{
		if (err)
		{
			ShowGError("%s", err);
			g_error_free(err);
		}

		g_free(uri);
		return FALSE;
	}

	g_free(uri);
	return TRUE;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);
	gchar *uri = g_filename_to_uri(LocalName, NULL, NULL);

	if (err)
		return FS_FILE_USERABORT;

	if (!gtk_recent_manager_add_item(manager, uri))
	{
		g_free(uri);
		return FS_FILE_WRITEERROR;
	}

	g_free(uri);
	return FS_FILE_OK;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;

}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	g_strlcpy(RemoteName, RemoteName + 1, maxlen - 1);
	return TRUE;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (g_strcmp0(Verb, "open") == 0)
	{
		//return FS_EXEC_YOURSELF;
		gchar *command = g_strdup_printf("xdg-open \"%s\"", RemoteName + 1);
		g_spawn_command_line_async(command, NULL);
		g_free(command);
		return FS_EXEC_OK;
	}

	return FS_EXEC_OK;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, _plugname, maxlen - 1);
}
