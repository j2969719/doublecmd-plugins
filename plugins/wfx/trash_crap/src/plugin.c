#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define SendDlgMsg gDialogApi->SendDlgMsg

typedef struct sVFSDirData
{
	GFile *gfile;
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
	{"original path",	ft_string,	"default|basename|dirname"},
	{"deletion date",	ft_string,				""},
};

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;


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

static void errmsg(const char *msg)
{
	if (gDialogApi)
		gDialogApi->MessageBox((char*)msg, "Double Commander", MB_OK | MB_ICONERROR);
	else if (gRequestProc)
		gRequestProc(gPluginNr, RT_MsgOK, NULL, (char*)msg, NULL, 0);
	else
		g_print("%s\n", msg);
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
	g_free(uri);

	GFileInfo *info = g_file_query_info(src, "trash::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (info)
	{
		const gchar *orgpath = g_file_info_get_attribute_byte_string(info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);
		GFile *dest = g_file_new_for_path(orgpath);

		if (!g_file_test(orgpath, G_FILE_TEST_EXISTS) || gRequestProc(gPluginNr, RT_MsgYesNo, NULL, "Overite?", NULL, 0))
		{
			if (!g_file_move(src, dest, G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA, NULL, NULL, NULL, &err))
			{
				if (err)
				{
					errmsg((err)->message);
					g_error_free(err);
				}
			}

		}

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
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;


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
	dirdata->gfile = g_file_new_for_uri(uri);
	g_free(uri);

	if (!G_IS_OBJECT(dirdata->gfile))
	{
		g_free(dirdata);
		return (HANDLE)(-1);
	}

	dirdata->enumer = g_file_enumerate_children(dirdata->gfile, "standard::*,unix::mode,trash::deletion-date,time::modified", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (dirdata->enumer && SetFindData(dirdata, FindData))
		return (HANDLE)dirdata;

	if (G_IS_OBJECT(dirdata->enumer))
		g_object_unref(dirdata->enumer);

	if (G_IS_OBJECT(dirdata->gfile))
		g_object_unref(dirdata->gfile);

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

	if (G_IS_OBJECT(dirdata->enumer))
	{
		g_file_enumerator_close(dirdata->enumer, NULL, NULL);
		g_object_unref(dirdata->enumer);
	}

	if (G_IS_OBJECT(dirdata->gfile))
		g_object_unref(dirdata->gfile);

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
	g_free(uri);
	g_file_delete(gfile, NULL, NULL);
	g_object_unref(gfile);

	return TRUE;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	gchar *uri = g_strdup_printf("trash://%s", RemoteName);
	GFile *gfile = g_file_new_for_uri(uri);
	g_free(uri);
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
	g_free(info->in_file);
	g_free(info->out_file);
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

		return FS_EXEC_OK;
	}

	return FS_EXEC_ERROR;
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
	const gchar *string = NULL;
	gchar *child = NULL;

	gchar **split = g_strsplit(FileName, "/", -1);

	if (g_strv_length(split) > 2)
	{
		gchar **target = split;
		target++;
		target++;
		gchar *temp = g_strjoinv("/", target);
		child = g_strdup_printf("/%s", temp);
		g_free(temp);
	}

	gchar *uri = g_strdup_printf("trash:///%s", split[1]);
	g_strfreev(split);
	GFile *src = g_file_new_for_uri(uri);
	g_free(uri);

	GFileInfo *info = g_file_query_info(src, "trash::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (info)
	{
		switch (FieldIndex)
		{
		case 0:
			string = g_file_info_get_attribute_byte_string(info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);

			if (string)
			{
				gchar *org = NULL;

				if (child)
					org = g_strjoin(NULL, string, child, NULL);
				else
					org = g_strdup(string);

				if (UnitIndex == 1)
				{
					gchar *filename = g_path_get_basename(org);
					g_strlcpy((char*)FieldValue, filename, maxlen - 1);
					g_free(filename);
				}
				else if (UnitIndex == 2)
				{
					gchar *folder = g_path_get_dirname(org);
					g_strlcpy((char*)FieldValue, folder, maxlen - 1);
					g_free(folder);
				}
				else
					g_strlcpy((char*)FieldValue, org, maxlen - 1);

				g_free(org);
			}
			else
				result = ft_fieldempty;

			break;

		case 1:
			string = g_file_info_get_attribute_string(info, G_FILE_ATTRIBUTE_TRASH_DELETION_DATE);

			if (string)
				g_strlcpy((char*)FieldValue, string, maxlen - 1);
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

	g_object_unref(src);
	g_free(child);

	return result;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	g_strlcpy(ViewContents, "[Plugin(FS).original path{}]", maxlen - 1);
	g_strlcpy(ViewHeaders, "Path", maxlen - 1);
	g_strlcpy(ViewWidths, "100,20,150", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Trash", maxlen - 1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gDialogApi == NULL)
	{
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gDialogApi != NULL)
		g_free(gDialogApi);

	gDialogApi = NULL;
}
