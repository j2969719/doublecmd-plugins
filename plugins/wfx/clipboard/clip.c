#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include "wfxplugin.h"

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
gint ienv;

const gchar *entries[] =
{
	"PRIMARY.TXT",
	"SECONDARY.TXT",
	"CLIPBOARD.TXT",
	"CLIPBOARD.PNG",
	NULL
};

gboolean selectionexist(int index)
{

	switch (index)
	{
	case 0:
		if (gtk_clipboard_wait_is_text_available(gtk_clipboard_get(GDK_SELECTION_PRIMARY)))
			return TRUE;

		break;

	case 1:
		if (gtk_clipboard_wait_is_text_available(gtk_clipboard_get(GDK_SELECTION_SECONDARY)))
			return TRUE;

		break;

	case 2:
		if (gtk_clipboard_wait_is_text_available(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)))
			return TRUE;

		break;

	case 3:
		if (gtk_clipboard_wait_is_image_available(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)))
			return TRUE;

		break;
	}

	return FALSE;

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

	if (entries[ienv] != NULL)
	{
		g_strlcpy(FindData->cFileName, entries[ienv], PATH_MAX);

		if (selectionexist(ienv))
			FindData->nFileSizeLow = 1;

		ienv++;
	}

	return (HANDLE)(1985);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (entries[ienv] != NULL)
	{
		g_strlcpy(FindData->cFileName, entries[ienv], PATH_MAX);

		if (selectionexist(ienv))
			FindData->nFileSizeLow = 1;

		ienv++;
		return TRUE;
	}
	else
		return FALSE;
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);

	if (err)
		return FS_FILE_USERABORT;

	if (access(LocalName, F_OK) == 0)
		return FS_FILE_NOTSUPPORTED; // FS_FILE_EXISTS;

	FILE* tfp = fopen(LocalName, "w");

	if (!tfp)
		return FS_FILE_WRITEERROR;

	if (g_strcmp0(RemoteName + 1, entries[0]) == 0)
	{

		fprintf(tfp, gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY)));
		fclose(tfp);
	}
	else if (g_strcmp0(RemoteName + 1, entries[1]) == 0)
	{
		fprintf(tfp, gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_SECONDARY)));
		fclose(tfp);
	}
	else if (g_strcmp0(RemoteName + 1, entries[2]) == 0)
	{
		fprintf(tfp, gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)));
		fclose(tfp);
	}
	else if (g_strcmp0(RemoteName + 1, entries[3]) == 0)
	{
		fclose(tfp);
		GdkPixbuf *pixbuf = gtk_clipboard_wait_for_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
		gdk_pixbuf_save(pixbuf, LocalName, "png", NULL, NULL);
	}

	return FS_FILE_NOTSUPPORTED;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (g_strcmp0(Verb, "open") == 0)
	{
		return FS_EXEC_YOURSELF;
	}

	return FS_EXEC_OK;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Clipboard", maxlen);
}
