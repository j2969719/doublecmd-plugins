#include <glib.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include "wfxplugin.h"

#define _plugname "Clipboard"
#define _pseudosize 1024

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
	"CLIPBOARD.JPG",
	"CLIPBOARD.BMP",
	"CLIPBOARD.ICO",
	NULL
};

static void ShowGError(gchar *str, GError *err)
{
	GtkWidget *dialog = gtk_message_dialog_new(NULL,
	                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s: %s", str, (err)->message);
	gtk_window_set_title(GTK_WINDOW(dialog), _plugname);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

gboolean SelectionExist(int index)
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

	default:
		if (gtk_clipboard_wait_is_image_available(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)))
			return TRUE;

		break;

	}

	return FALSE;

}


void GetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

gboolean SetFindData(WIN32_FIND_DATAA *FindData)
{
	gboolean found = FALSE;
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while (entries[ienv] != NULL)
	{
		ienv++;

		if (SelectionExist(ienv - 1))
		{
			found = TRUE;
			break;
		}
	}

	if (found)
	{
		FindData->nFileSizeLow = _pseudosize;
		g_strlcpy(FindData->cFileName, entries[ienv - 1], PATH_MAX);
		GetCurrentFileTime(&FindData->ftLastWriteTime);
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->dwFileAttributes = 0;
		return found;
	}

	return found;
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
	ienv = 0;

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
	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);
	GError *gerr = NULL;

	if (err)
		return FS_FILE_USERABORT;

	if ((CopyFlags == 0) && (access(LocalName, F_OK) == 0))
		return FS_FILE_EXISTS;

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

		if (!SelectionExist(3))
		{
			remove(LocalName);
			return FS_FILE_NOTFOUND;
		}

		GdkPixbuf *pixbuf = gtk_clipboard_wait_for_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));

		if (pixbuf)
			gdk_pixbuf_save(pixbuf, LocalName, "png", &gerr, NULL);
	}
	else if (g_strcmp0(RemoteName + 1, entries[4]) == 0)
	{
		fclose(tfp);

		if (!SelectionExist(4))
		{
			remove(LocalName);
			return FS_FILE_NOTFOUND;
		}

		GdkPixbuf *pixbuf = gtk_clipboard_wait_for_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));

		if (pixbuf)
			gdk_pixbuf_save(pixbuf, LocalName, "jpeg", &gerr, NULL);
	}
	else if (g_strcmp0(RemoteName + 1, entries[5]) == 0)
	{
		fclose(tfp);

		if (!SelectionExist(5))
		{
			remove(LocalName);
			return FS_FILE_NOTFOUND;
		}

		GdkPixbuf *pixbuf = gtk_clipboard_wait_for_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));

		if (pixbuf)
			gdk_pixbuf_save(pixbuf, LocalName, "bmp", &gerr, NULL);
	}
	else if (g_strcmp0(RemoteName + 1, entries[6]) == 0)
	{
		fclose(tfp);

		if (!SelectionExist(6))
		{
			remove(LocalName);
			return FS_FILE_NOTFOUND;
		}

		GdkPixbuf *pixbuf = gtk_clipboard_wait_for_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));

		if (pixbuf)
			gdk_pixbuf_save(pixbuf, LocalName, "ico", &gerr, NULL);
	}
	else
	{
		fclose(tfp);
		remove(LocalName);
		return FS_FILE_NOTFOUND;
	}



	if (gerr)
	{
		ShowGError(LocalName, gerr);
		g_error_free(gerr);
	}

	return FS_FILE_OK;
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
	g_strlcpy(DefRootName, _plugname, maxlen);
}
