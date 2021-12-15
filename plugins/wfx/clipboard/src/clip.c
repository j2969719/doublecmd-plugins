#include <glib.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <errno.h>
#include <string.h>
#include "wfxplugin.h"

typedef struct sVFSDirData
{
	gint i;
} tVFSDirData;

typedef struct sField
{
	char *name;
	int type;
	char *unit;
} tField;

#define fieldcount (sizeof(fields)/sizeof(tField))

tField fields[] =
{
	{"Text Preview",	ft_string,			""},
	{"Content",	ft_multiplechoice,	 "uris|image|text"},
};

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;

gboolean gSetContent;
GtkClipboard *gClipboard;

const gchar *gEntries[] =
{
	"PRIMARY.TXT",
	"SECONDARY.TXT",
	"CLIPBOARD.TXT",
	"CLIPBOARD.png",
	"CLIPBOARD.jpeg",
	"CLIPBOARD.bmp",
	"CLIPBOARD.ico",
	NULL
};

static gboolean SelectionExist(int index)
{
	switch (index)
	{
	case 0:
		gClipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);

		return gtk_clipboard_wait_is_text_available(gClipboard);

	case 1:
		gClipboard = gtk_clipboard_get(GDK_SELECTION_SECONDARY);

		return gtk_clipboard_wait_is_text_available(gClipboard);

	case 2:
		gClipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

		return gtk_clipboard_wait_is_text_available(gClipboard);

	default:
		gClipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

		return gtk_clipboard_wait_is_image_available(gClipboard);
	}

	return FALSE;
}

static void GetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

static gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	gboolean found = FALSE;
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while (gEntries[dirdata->i] != NULL && !found)
	{
		found = SelectionExist(dirdata->i);
		dirdata->i++;
	}

	if (found)
	{
		FindData->nFileSizeLow = 1024;
		g_strlcpy(FindData->cFileName, gEntries[dirdata->i - 1], MAX_PATH);
		GetCurrentFileTime(&FindData->ftCreationTime);
		GetCurrentFileTime(&FindData->ftLastAccessTime);
		GetCurrentFileTime(&FindData->ftLastWriteTime);
		FindData->dwFileAttributes = 0;
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
	tVFSDirData *dirdata;

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	dirdata->i = 0;

	if (SetFindData(dirdata, FindData))
		return (HANDLE)dirdata;

	g_free(dirdata);

	return (HANDLE)(-1);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return SetFindData(dirdata, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	g_free(dirdata);

	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	gchar *contents = NULL;
	GdkPixbuf *pixbuf = NULL;
	GError *err = NULL;

	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0) == 1)
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	if (g_strcmp0(RemoteName + 1, gEntries[0]) == 0)
	{
		gClipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		contents = gtk_clipboard_wait_for_text(gClipboard);

		if (contents)
			g_file_set_contents(LocalName, contents, -1, &err);
		else
			return FS_FILE_NOTFOUND;
	}
	else if (g_strcmp0(RemoteName + 1, gEntries[1]) == 0)
	{
		gClipboard = gtk_clipboard_get(GDK_SELECTION_SECONDARY);
		contents = gtk_clipboard_wait_for_text(gClipboard);

		if (contents)
			g_file_set_contents(LocalName, contents, -1, &err);
		else
			return FS_FILE_NOTFOUND;
	}
	else if (g_strcmp0(RemoteName + 1, gEntries[2]) == 0)
	{
		gClipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		contents = gtk_clipboard_wait_for_text(gClipboard);

		if (contents)
			g_file_set_contents(LocalName, contents, -1, &err);
		else
			return FS_FILE_NOTFOUND;
	}
	else
	{
		gClipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		pixbuf = gtk_clipboard_wait_for_image(gClipboard);

		if (pixbuf)
		{
			if (SelectionExist(3))
			{

				char *dot = strchr(RemoteName, '.');

				if (dot)
					gdk_pixbuf_save(pixbuf, LocalName, dot + 1, &err, NULL);
			}

			g_object_unref(pixbuf);
		}
		else
			return FS_FILE_NOTFOUND;
	}

	g_free(contents);

	if (err)
	{
		gRequestProc(gPluginNr, RT_MsgOK, NULL, (err)->message, NULL, 0);
		g_error_free(err);
		return FS_FILE_WRITEERROR;
	}

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	return FS_FILE_OK;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	gchar *contents = NULL;

	gClipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0) == 1)
		return FS_FILE_USERABORT;

	if (gSetContent)
	{
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(LocalName, NULL);

		if (pixbuf)
		{
			gtk_clipboard_set_image(gClipboard, pixbuf);
			g_object_unref(pixbuf);
		}
		else
		{
			if (g_file_get_contents(LocalName, &contents, NULL, NULL))
				gtk_clipboard_set_text(gClipboard, contents, -1);
			else
				return FS_FILE_WRITEERROR;
		}
	}
	else
	{
		gchar *temp = g_filename_to_uri(LocalName, NULL, NULL);
		gchar *uri = g_uri_unescape_string(temp, NULL);
		g_free(temp);
		contents = gtk_clipboard_wait_for_text(gClipboard);

		if (contents)
		{
			temp = g_strdup_printf("%s\n%s", contents, uri);
			gtk_clipboard_set_text(gClipboard, temp, -1);
			g_free(temp);
		}
		else
			gtk_clipboard_set_text(gClipboard, uri, -1);

		g_free(uri);
	}

	g_free(contents);

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	return FS_FILE_OK;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	if (g_strcmp0(RemoteName + 1, gEntries[0]) == 0)
	{
		gClipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		gtk_clipboard_clear(gClipboard);
	}
	else if (g_strcmp0(RemoteName + 1, gEntries[1]) == 0)
	{
		gClipboard = gtk_clipboard_get(GDK_SELECTION_SECONDARY);
		gtk_clipboard_clear(gClipboard);
	}
	else
	{
		gClipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		gtk_clipboard_clear(gClipboard);
	}

	return TRUE;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strncmp(Verb, "open", 5) == 0)
		return FS_EXEC_YOURSELF;
	else if (gRequestProc)
		gRequestProc(gPluginNr, RT_MsgOK, NULL, strerror(EOPNOTSUPP), NULL, 0);

	return FS_EXEC_OK;
}


int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	g_strlcpy(Units, fields[FieldIndex].unit, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int result = fields[FieldIndex].type;
	gchar *strvalue = NULL;

	if (g_strcmp0(FileName + 1, gEntries[0]) == 0)
		gClipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	else if (g_strcmp0(FileName + 1, gEntries[1]) == 0)
		gClipboard = gtk_clipboard_get(GDK_SELECTION_SECONDARY);
	else
		gClipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

	switch (FieldIndex)
	{
	case 0:
		if (gtk_clipboard_wait_is_image_available(gClipboard))
			result = ft_fieldempty;
		else
		{
			strvalue = gtk_clipboard_wait_for_text(gClipboard);

			if (strvalue)
				g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
			else
				result = ft_fieldempty;
		}

		break;

	case 1:
		if (gtk_clipboard_wait_is_uris_available(gClipboard))
			g_strlcpy((char*)FieldValue, "uris", maxlen - 1);
		else if (gtk_clipboard_wait_is_image_available(gClipboard))
			g_strlcpy((char*)FieldValue, "image", maxlen - 1);
		else if (gtk_clipboard_wait_is_text_available(gClipboard))
			g_strlcpy((char*)FieldValue, "text", maxlen - 1);
		else
			result = ft_fieldempty;

		break;

	default:
		result = ft_nosuchfield;
	}

	g_free(strvalue);

	return result;
}

void DCPCALL FsStatusInfo(char* RemoteDir, int InfoStartEnd, int InfoOperation)
{
	if (InfoOperation == FS_STATUS_OP_PUT_SINGLE)
		if (InfoStartEnd == FS_STATUS_END)
			gSetContent = FALSE;
		else
			gSetContent = gRequestProc(gPluginNr, RT_MsgYesNo, NULL, "Сopy file contents to CLIPBOARD?", NULL, 0);
}
/*
BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	g_strlcpy(ViewContents, "[Plugin(FS).Content{}]\\n[Plugin(FS).Text Preview{}]", maxlen - 1);
	g_strlcpy(ViewHeaders, "Content\\nPreview", maxlen - 1);
	g_strlcpy(ViewWidths, "100,30,30,100", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
}
*/
void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Clipboard", maxlen - 1);
}
