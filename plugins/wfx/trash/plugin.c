#define _GNU_SOURCE
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))

typedef struct sVFSDirData
{
	GFileEnumerator *enumer;
} tVFSDirData;

typedef struct sCopyInfo
{
	gchar *in_file;
	gchar *out_file;
} tCopyInfo;

typedef struct sField
{
	char *name;
	int type;
	char *unit;
} tField;

#define fieldcount (sizeof(fields)/sizeof(tField))

tField fields[] =
{
	{"original path",	ft_string,	""},
	{"deletion date",	ft_string,	""},
};

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;


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


static void SetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

static void try_free_str(gchar *str)
{
	if (str)
		g_free(str);
}

static void errmsg(const char *msg)
{
	gRequestProc(gPluginNr, RT_MsgOK, NULL, msg ? (char*)msg : "Unknown error", NULL, 0);
}

static void copy_progress_cb(goffset current_num_bytes, goffset total_num_bytes, gpointer user_data)
{
	tCopyInfo *info = (tCopyInfo*)user_data;

	gint64 res = 0;

	if (total_num_bytes > 0)
		res = current_num_bytes * 100 / total_num_bytes;

	gProgressProc(gPluginNr, info->in_file, info->out_file, res);
}

static void restore_from_trash(const char *remote)
{
	GError *err = NULL;
	gchar *uri = g_strdup_printf("trash://%s", remote);
	GFile *src = g_file_new_for_uri(uri);
	try_free_str(uri);

	GFileInfo *info = g_file_query_info(src, "trash::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (info)
	{
		const gchar *orgpath = g_file_info_get_attribute_byte_string(info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);
		GFile *dest = g_file_new_for_path(orgpath);

		if (g_file_test(orgpath, G_FILE_TEST_EXISTS) && !gRequestProc(gPluginNr, RT_MsgYesNo, NULL, "Overite?", NULL, 0))
		{

		}
		else if (!g_file_copy(src, dest, G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA, NULL, NULL, NULL, &err))
		{
			if (err)
			{
				errmsg((err)->message);
				g_error_free(err);
			}
		}
		else
			g_file_delete(src, NULL, NULL);

		g_object_unref(info);
		g_object_unref(dest);
	}

	g_object_unref(src);
}

gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	GFileInfo *info = g_file_enumerator_next_file(dirdata->enumer, NULL, NULL);

	if (info)
	{
		g_strlcpy(FindData->cFileName, g_file_info_get_name(info), sizeof(FindData->cFileName) - 1);

		if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY)
			FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		else
		{
			goffset size = g_file_info_get_size(info);
			FindData->nFileSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = size & 0x00000000FFFFFFFF;
		}

		GDateTime *deletion_date = g_file_info_get_deletion_date(info);
		gint64 unix_time;

		if (deletion_date)
			unix_time = g_date_time_to_unix(deletion_date);
		else
			unix_time = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_MODIFIED);

		UnixTimeToFileTime(unix_time, &FindData->ftLastWriteTime);
		UnixTimeToFileTime(unix_time, &FindData->ftLastAccessTime);
		UnixTimeToFileTime(unix_time, &FindData->ftCreationTime);

		FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
		FindData->dwReserved0 = (DWORD)g_file_info_get_attribute_uint32(info, G_FILE_ATTRIBUTE_UNIX_MODE);

		g_object_unref(info);
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

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	gchar *uri = g_strdup_printf("trash://%s", Path);
	GFile *gfile = g_file_new_for_uri(uri);
	try_free_str(uri);

	dirdata->enumer = g_file_enumerate_children(gfile, "standard::*,unix::mode,trash::deletion-date,time::modified", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (dirdata->enumer && SetFindData(dirdata, FindData))
		return (HANDLE)dirdata;

	g_free(dirdata);

	return (HANDLE)(-1);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	if (SetFindData(dirdata, FindData))
		return TRUE;
	else
		return FALSE;
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	g_file_enumerator_close(dirdata->enumer, NULL, NULL);
	g_object_unref(dirdata->enumer);
	g_free(dirdata);

	return 0;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int result = FS_FILE_OK;
	GError *err = NULL;

	GFile *gfile = g_file_new_for_path(LocalName);

	if (gProgressProc(gPluginNr, "trash:///", LocalName, 0))
		return FS_FILE_USERABORT;

	if (!g_file_trash(gfile, NULL, &err))
		result = FS_FILE_WRITEERROR;

	if (err)
	{
		errmsg((err)->message);
		g_error_free(err);
	}

	g_object_unref(gfile);
	gProgressProc(gPluginNr, "trash:///", LocalName, 100);

	return result;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	return TRUE;
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	gchar *uri = g_strdup_printf("trash://%s", RemoteName);
	GFile *gfile = g_file_new_for_uri(uri);
	try_free_str(uri);
	g_file_delete(gfile, NULL, NULL);
	g_object_unref(gfile);

	return TRUE;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	gchar *uri = g_strdup_printf("trash://%s", RemoteName);
	GFile *gfile = g_file_new_for_uri(uri);
	try_free_str(uri);
	g_file_delete(gfile, NULL, NULL);
	g_object_unref(gfile);

	return TRUE;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int result = FS_FILE_OK;
	GError *err = NULL;

	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0))
		return FS_FILE_USERABORT;

	tCopyInfo *info = g_new0(tCopyInfo, 1);

	info->in_file = g_strdup_printf("trash://%s", RemoteName);
	info->out_file = g_strdup(LocalName);

	if (CopyFlags == 0 && g_file_test(info->out_file, G_FILE_TEST_EXISTS))
	{
		g_free(info);
		return FS_FILE_EXISTS;
	}

	GFile *src = g_file_new_for_uri(info->in_file);
	GFile *dest = g_file_new_for_path(info->out_file);

	if (!g_file_copy(src, dest, G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA, NULL, copy_progress_cb, (gpointer)info, &err))
	{
		result = FS_FILE_WRITEERROR;

		if (err)
		{
			errmsg((err)->message);
			g_error_free(err);
		}
	}

	g_object_unref(src);
	g_object_unref(dest);
	g_free(info);

	return result;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strncmp(Verb, "open", 4) == 0)
		return FS_EXEC_YOURSELF;
	else if (RemoteName[1] != '\0' && strncmp(Verb, "properties", 10) == 0)
	{
		if (gRequestProc(gPluginNr, RT_MsgYesNo, NULL, "restore?", NULL, 0))
			restore_from_trash(RemoteName);
	}
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
	const gchar *strvalue = NULL;

	gchar *uri = g_strdup_printf("trash://%s", FileName);
	GFile *src = g_file_new_for_uri(uri);
	try_free_str(uri);

	GFileInfo *info = g_file_query_info(src, "trash::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (info)
	{
		switch (FieldIndex)
		{
		case 0:
			strvalue = g_file_info_get_attribute_byte_string(info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);

			if (strvalue)
				g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
			else
				result = ft_fieldempty;

			break;

		case 1:
			strvalue = g_file_info_get_attribute_string(info, G_FILE_ATTRIBUTE_TRASH_DELETION_DATE);

			if (strvalue)
				g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
			else
				result = ft_fieldempty;

			break;

		default:
			result = ft_nosuchfield;
		}

		g_object_unref(info);
	}
	else
		result = ft_fieldempty;

	return result;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Trash", maxlen - 1);
}
