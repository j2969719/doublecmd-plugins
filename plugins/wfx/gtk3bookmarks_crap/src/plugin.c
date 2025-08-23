#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include "extension.h"
#include "wfxplugin.h"

#define ROOTNAME "Bookmarks (GTK3)"
#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox

typedef struct sVFSDirData
{
	gsize index;
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;

gchar gBookmarkFile[PATH_MAX] = "";
gchar **gBookmarks = NULL;
GData *gFileList = NULL;

gboolean UnixTimeToFileTime(unsigned long mtime, LPFILETIME ft)
{
	gint64 ll = Int32x32To64(mtime, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
	return TRUE;
}

gchar* MakeFileName(gchar *path, const gchar *host, char *pos)
{
	gchar *result = NULL;
	gchar *filename = NULL;

	if (pos)
		filename = pos + 1;

	if (filename && filename[0] != '\0')
	{
		filename = g_strdup(pos + 1);
	}
	else if (!host)
	{
		if (strcmp(path, "/") != 0)
			filename = g_path_get_basename(path);
		else
			return NULL;
	}
	else
	{
		if (strcmp(path, "/") != 0)
		{
			gchar *name = g_path_get_basename(path);
			filename = g_strdup_printf("%s %s", host, name);
			g_free(name);
		}
		else
			filename = g_strdup_printf("%s", host);
	}

	int num = 1;
	result = g_strdup_printf("%s.%d", filename, num);

	while ((gchar*)g_datalist_get_data(&gFileList, result) != NULL)
	{
		g_free(result);
		result = g_strdup_printf("%s.%d", filename, ++num);
	}

	g_free(filename);

	return result;
}

gboolean FillFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	gchar *line = NULL;

	while ((line = gBookmarks[dirdata->index]) != NULL)
	{
		gsize index = dirdata->index;
		dirdata->index++;

		char *pos = strrchr(line, ' ');

		if (pos) 
			*pos = '\0';

		if (!g_uri_is_valid(line, 0, NULL))
			continue;

		memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
		FindData->nFileSizeHigh = 0xFFFFFFFF;
		FindData->nFileSizeLow = 0xFFFFFFFE;
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;

		if (strncmp(line, "file://", 6) == 0)
		{
			struct stat buf;
			gchar *path = g_filename_from_uri(line, NULL, NULL);

			if (path)
			{
				g_free(gBookmarks[index]);
				gBookmarks[index] = path;
			}
			else
				continue;

			gchar *name = MakeFileName(path, NULL, pos);

			if (!name)
				continue;

			g_strlcpy(FindData->cFileName, name, MAX_PATH);
			g_datalist_set_data(&gFileList, name, (gpointer)gBookmarks[index]);
			g_free(name);

			if (stat(path, &buf) == 0)
			{
				UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
				FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
				FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;
				FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
				FindData->dwReserved0 = buf.st_mode & ~S_IFDIR;
			}

			return TRUE;
		}
		else
		{
			GUri *uri = g_uri_parse(line, 0, NULL);

			if (!uri)
				continue;

			const gchar *host = g_uri_get_host(uri);
			const gchar *path = g_uri_get_path(uri);
			gchar *name = MakeFileName((gchar*)path, host, pos);

			if (!name)
			{
				g_uri_unref(uri);
				continue;
			}

			g_strlcpy(FindData->cFileName, name, MAX_PATH);
			g_datalist_set_data(&gFileList, name, (gpointer)gBookmarks[index]);
			g_free(name);

			g_uri_unref(uri);
			return TRUE;
		}
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
	gchar *text = NULL;

	if (!gBookmarkFile)
		return (HANDLE)(-1);

	if (!g_file_get_contents(gBookmarkFile, &text, NULL, NULL))
		return (HANDLE)(-1);

	tVFSDirData *dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
	{
		g_free(text);
		return (HANDLE)(-1);
	}

	if (gBookmarks)
		g_strfreev(gBookmarks);

	g_datalist_clear(&gFileList);
	g_datalist_init(&gFileList);
	gBookmarks = g_strsplit(text, "\n", -1);
	g_free(text);


	if (FillFindData(dirdata, FindData))
		return (HANDLE)dirdata;

	g_free(dirdata);

	return (HANDLE)(-1);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return FillFindData(dirdata, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	g_free(dirdata);

	return 0;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	int result = FS_EXEC_ERROR;

	if (!gBookmarks)
		return result;

	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, RemoteName + 1);

	if (!uri)
		return result;

	if (strcmp(Verb, "open") == 0)
	{
		if (uri[0] == '/' && !g_file_test(uri, G_FILE_TEST_IS_DIR))
			return FS_EXEC_OK;
			
		g_strlcpy(RemoteName, uri, MAX_PATH);
		result = FS_EXEC_SYMLINK;
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		MessageBox((char*)uri, ROOTNAME,  MB_OK | MB_ICONINFORMATION);
		result = FS_EXEC_OK;
	}

	return result;
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex > 0)
		return ft_nomorefields;

	g_strlcpy(FieldName, "Bookmark", maxlen - 1);
	Units[0] = '\0';

	return ft_string;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (!gBookmarks)
		return ft_fieldempty;

	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, FileName + 1);

	if (uri)
	{
		g_strlcpy((char*)FieldValue, uri, maxlen - 1);
		return ft_string;
	}

	return ft_fieldempty;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	g_strlcpy(ViewContents, "[Plugin(FS).Bookmark{}]", maxlen - 1);
	g_strlcpy(ViewHeaders, "->", maxlen - 1);
	g_strlcpy(ViewWidths, "100,0,250", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);

	return TRUE;
}

int DCPCALL FsExtractCustomIcon(char* RemoteName, int ExtractFlags, PWfxIcon TheIcon)
{
	if (!gBookmarks)
		return FS_ICON_USEDEFAULT;

	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, RemoteName + 1);

	if (uri[0] == '/')
	{
		if (!g_file_test(uri, G_FILE_TEST_EXISTS))
			g_strlcpy(RemoteName, "user-trash", MAX_PATH);
		else if (g_file_test(uri, G_FILE_TEST_IS_DIR))
			g_strlcpy(RemoteName, "folder", MAX_PATH);
		else
			g_strlcpy(RemoteName, "face-surprise", MAX_PATH);
	}
	else
		g_strlcpy(RemoteName, "folder-remote", MAX_PATH);

	TheIcon->Format = FS_ICON_FORMAT_FILE;

	return FS_ICON_EXTRACTED;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, ROOTNAME, maxlen - 1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo * StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
		snprintf(gBookmarkFile, PATH_MAX, "%s/.config/gtk-3.0/bookmarks", g_getenv("HOME"));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
	{
		free(gExtensions);
		gExtensions = NULL;
	}

	if (gBookmarks)
		g_strfreev(gBookmarks);
}
