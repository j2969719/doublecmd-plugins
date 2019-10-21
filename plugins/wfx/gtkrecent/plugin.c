#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <fcntl.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "extension.h"
#include "wfxplugin.h"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))

typedef struct sVFSDirData
{
	GList *list;
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
	{"Basename",		ft_string,	""},
	{"BasenameNoExt",	ft_string,	""},
	{"Dirname",		ft_string,	""},
};

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
GtkRecentManager *gManager;

gboolean UnixTimeToFileTime(unsigned long mtime, LPFILETIME ft)
{
	gint64 ll = Int32x32To64(mtime, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
	return TRUE;
}

unsigned long FileTimeToUnixTime(LPFILETIME ft)
{
	gint64 ll = ft->dwHighDateTime;
	ll = (ll << 32) | ft->dwLowDateTime;
	ll = (ll - 116444736000000000) / 10000000;
	return (unsigned long)ll;
}

static gchar *basenamenoext(const gchar *file)
{
	gchar *basename = g_path_get_basename(file);
	gchar *result = g_strdup(basename);
	gchar *dot = g_strrstr(result, ".");

	if (dot)
	{
		int offset = dot - result;
		result[offset] = '\0';

		if (result[0] == '\0')
			result = g_strdup(basename);
	}

	g_free(basename);
	return result;
}

gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	GList *list;
	gchar *filename = NULL;
	unsigned long atime;
	struct stat buf;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	list = dirdata->list;

	if (list != NULL)
	{
		do
		{
			filename = g_filename_from_uri(gtk_recent_info_get_uri(list->data), NULL, NULL);
			atime = gtk_recent_info_get_visited(list->data);
			gtk_recent_info_unref(list->data);
			list = list->next;
		}
		while (!filename && list != NULL);

		if (!filename)
			return FALSE;

		g_strlcpy(FindData->cFileName, filename, PATH_MAX);

		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;

		if (atime != -1)
			UnixTimeToFileTime(atime, &FindData->ftLastAccessTime);

		if (stat(filename, &buf) != 0)
			FindData->nFileSizeLow = 0;
		else
		{
			FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;

			if (!S_ISDIR(buf.st_mode))
			{
				FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
				FindData->dwReserved0 = buf.st_mode;
			}

			UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);

			if (atime != -1)
				UnixTimeToFileTime(buf.st_atime, &FindData->ftLastAccessTime);

		}

		dirdata->list = list;

		g_free(filename);
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
	gManager = gtk_recent_manager_get_default();

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	dirdata->list = gtk_recent_manager_get_items(gManager);

	if (SetFindData(dirdata, FindData))
		return (HANDLE)dirdata;
	else
	{
		if (dirdata->list != NULL)
			g_list_free(dirdata->list);

		g_free(dirdata);

		return (HANDLE)(-1);
	}
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return SetFindData(dirdata, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	if (dirdata->list != NULL)
		g_list_free(dirdata->list);

	g_free(dirdata);

	return 0;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	gchar *uri = g_filename_to_uri(RemoteName + 1, NULL, NULL);
	GError *err = NULL;

	if (!uri)
		return FALSE;

	if (!gtk_recent_manager_remove_item(gManager, uri, &err))
	{
		uri = g_strdup_printf("file:/%s", RemoteName);

		if (err && !gtk_recent_manager_remove_item(gManager, uri, NULL))
			gRequestProc(gPluginNr, RT_MsgOK, NULL, (err)->message, NULL, 0);
	}

	if (err)
		g_error_free(err);

	g_free(uri);

	return TRUE;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);
	gchar *uri = g_filename_to_uri(LocalName, NULL, NULL);

	if (err)
		return FS_FILE_USERABORT;

	if (!gtk_recent_manager_add_item(gManager, uri))
	{
		g_free(uri);
		return FS_FILE_WRITEERROR;
	}

	g_free(uri);

	return FS_FILE_OK;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	return TRUE;
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
	int result = FS_EXEC_OK;

	if (strncmp(Verb, "open", 5) == 0)
	{
		//rresult = FS_EXEC_YOURSELF;
		gchar *command = g_strdup_printf("xdg-open %s", g_shell_quote(RemoteName + 1));
		g_spawn_command_line_async(command, NULL);
		g_free(command);
		//g_strlcpy(RemoteName, g_strdup(RemoteName + 1), PATH_MAX);
		//result = FS_EXEC_SYMLINK;
	}
	else if (strncmp(Verb, "chmod", 5) == 0)
	{
		int mode = strtoll(Verb + 6, 0, 8);

		if (g_chmod(RemoteName + 1, mode) == -1)
			result = FS_EXEC_ERROR;
	}
	else
		gRequestProc(gPluginNr, RT_MsgOK, NULL, strerror(EOPNOTSUPP), NULL, 0);

	return FS_EXEC_OK;
}

BOOL DCPCALL FsSetTime(char* RemoteName, FILETIME *CreationTime, FILETIME *LastAccessTime, FILETIME *LastWriteTime)
{
	struct stat buf;
	struct utimbuf ubuf;

	if (LastAccessTime != NULL || LastWriteTime != NULL)
	{

		if (g_stat(RemoteName + 1, &buf) == 0)
		{
			if (LastAccessTime != NULL)
				ubuf.actime = FileTimeToUnixTime(LastAccessTime);
			else
				ubuf.actime = buf.st_atime;

			if (LastWriteTime != NULL)
				ubuf.modtime = FileTimeToUnixTime(LastWriteTime);
			else
				ubuf.modtime = buf.st_mtime;

			if (utime(RemoteName + 1, &ubuf) == 0)
				return TRUE;
		}
	}

	return FALSE;
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

	switch (FieldIndex)
	{
	case 0:
		strvalue = g_path_get_basename(FileName);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			result = ft_fieldempty;
		break;
	case 1:
		strvalue = basenamenoext(FileName);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			result = ft_fieldempty;
		break;
	case 3:
		strvalue = g_path_get_dirname(FileName);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			result = ft_fieldempty;
		break;
	default:
		result = ft_nosuchfield;
	}

	if (strvalue)
		g_free(strvalue);

	return result;
}


void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "GTKRecent", maxlen - 1);
}
