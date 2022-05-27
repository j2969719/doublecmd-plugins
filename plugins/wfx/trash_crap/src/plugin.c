#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define SendDlgMsg gDialogApi->SendDlgMsg

typedef struct sVFSDirData
{
	GFile *gfile;
	GFileEnumerator *enumer;
} tVFSDirData;

typedef struct sCopyInfo
{
	gchar in_file[PATH_MAX];
	gchar out_file[PATH_MAX];
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
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;
char gLastFile[PATH_MAX];
char gLFMPath[PATH_MAX];


gboolean UnixTimeToFileTime(unsigned long mtime, LPFILETIME ft)
{
	gint64 ll = Int32x32To64(mtime, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
	return TRUE;
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

static GFile *filename_to_gfile(char *filename)
{
	GFile *result = NULL;
	char *string = NULL;

#ifdef GLIB_VERSION_2_66
	GUri *uri = g_uri_build(G_URI_FLAGS_NONE, "trash", NULL, NULL, -1, filename, NULL, NULL);
	string = g_uri_to_string(uri);
	g_uri_unref(uri);
#else
	char *temp = g_uri_escape_string(filename, "/", TRUE);

	if (temp[0] == '/')
		string = g_strdup_printf("trash://%s", temp);
	else
		string = g_strdup_printf("trash:///%s", temp);

	g_free(temp);

#endif
	result = g_file_new_for_uri(string);
	g_free(string);
	return result;
}

static GFile *get_delobject(gchar *filename)
{
	GFile *result = NULL;
	gchar **split = g_strsplit(filename, "/", -1);
	result = filename_to_gfile(split[1]);
	g_strfreev(split);
	return result;
}

static gboolean restore_from_trash(gchar *filename)
{
	GError *err = NULL;
	GFile *src = get_delobject(filename);
	GFileInfo *info = g_file_query_info(src, "standard::*,trash::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (info)
	{
		gchar *msg = g_strdup_printf(_("Restore %s from trash?"), g_file_info_get_display_name(info));

		if (!gRequestProc(gPluginNr, RT_MsgYesNo, NULL, msg, NULL, 0))
		{
			g_object_unref(info);
			g_object_unref(src);
			g_free(msg);
			return FALSE;
		}

		g_free(msg);

		const gchar *orgpath = g_file_info_get_attribute_byte_string(info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);
		GFile *dest = g_file_new_for_path(orgpath);

		if (!g_file_test(orgpath, G_FILE_TEST_EXISTS) || gRequestProc(gPluginNr, RT_MsgYesNo, NULL, _("Already exists, overwrite?"), NULL, 0))
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
	else
	{
		errmsg(_("Failed to get information about the removed object."));
		g_object_unref(src);
		return FALSE;
	}

	g_object_unref(src);
	return TRUE;
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
	{
		SendDlgMsg(pDlg, "lblDelPath", DM_SETTEXT, (intptr_t)_("Original Path:"), 0);
		SendDlgMsg(pDlg, "lblDelDateDscr", DM_SETTEXT, (intptr_t)_("Deletion Date:"), 0);
		GFile *src = get_delobject(gLastFile);
		GFileInfo *info = g_file_query_info(src, "standard::*,time::*,trash::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

		if (!info)
		{
			SendDlgMsg(pDlg, "btnRestore", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "edPath", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "edName", DM_SETTEXT, (intptr_t)_("Unknown"), 0);
			SendDlgMsg(pDlg, "edPath", DM_SETTEXT, (intptr_t)_("Unknown"), 0);
			SendDlgMsg(pDlg, "lblDelDate", DM_SETTEXT, (intptr_t)_("Unknown"), 0);
			SendDlgMsg(pDlg, "lblSize", DM_SETTEXT, (intptr_t)_("Unknown"), 0);
			SendDlgMsg(pDlg, "lblType", DM_SETTEXT, (intptr_t)_("Unknown"), 0);
			SendDlgMsg(pDlg, "lblModtime", DM_SETTEXT, (intptr_t)_("Unknown"), 0);
			SendDlgMsg(pDlg, "lblAccess", DM_SETTEXT, (intptr_t)_("Unknown"), 0);
			SendDlgMsg(pDlg, "lblChange", DM_SETTEXT, (intptr_t)_("Unknown"), 0);
			errmsg(_("Failed to get information about the removed object."));
		}
		else
		{
			const char *path = g_file_info_get_attribute_byte_string(info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);
			SendDlgMsg(pDlg, "edName", DM_SETTEXT, (intptr_t)g_file_info_get_display_name(info), 0);
			SendDlgMsg(pDlg, "edPath", DM_SETTEXT, (intptr_t)path, 0);
			const char *date = g_file_info_get_attribute_string(info, G_FILE_ATTRIBUTE_TRASH_DELETION_DATE);
			SendDlgMsg(pDlg, "lblDelDate", DM_SETTEXT, (intptr_t)date, 0);
			gchar *size = g_format_size(g_file_info_get_size(info));
			SendDlgMsg(pDlg, "lblSize", DM_SETTEXT, (intptr_t)size, 0);
			const char *content = g_file_info_get_content_type(info);
			gchar *descr = g_content_type_get_description(content);
			SendDlgMsg(pDlg, "lblType", DM_SETTEXT, (intptr_t)descr, 0);
			g_free(descr);
			const char *target = g_file_info_get_symlink_target(info);

			if (target)
			{
				SendDlgMsg(pDlg, "lblInfo2", DM_SHOWITEM, 1, 0);
				SendDlgMsg(pDlg, "edSymlink", DM_SHOWITEM, 1, 0);
				SendDlgMsg(pDlg, "edSymlink", DM_SETTEXT, (intptr_t)target, 0);
			}

			guint64 unix_time = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
			gchar *datetime = g_date_time_format(g_date_time_new_from_unix_local(unix_time), "%Y-%m-%dT%H:%M:%S");
			SendDlgMsg(pDlg, "lblModtime", DM_SETTEXT, (intptr_t)datetime, 0);
			g_free(datetime);
			unix_time = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_ACCESS);
			datetime = g_date_time_format(g_date_time_new_from_unix_local(unix_time), "%Y-%m-%dT%H:%M:%S");
			SendDlgMsg(pDlg, "lblAccess", DM_SETTEXT, (intptr_t)datetime, 0);
			g_free(datetime);
			unix_time = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_CHANGED);
			datetime = g_date_time_format(g_date_time_new_from_unix_local(unix_time), "%Y-%m-%dT%H:%M:%S");
			SendDlgMsg(pDlg, "lblChange", DM_SETTEXT, (intptr_t)datetime, 0);
			g_free(datetime);
			g_object_unref(info);
		}

		g_object_unref(src);
	}

	break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnRestore") == 0)
		{
			if (restore_from_trash(gLastFile))
				SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_OK, 0);
		}

		break;
	}

	return 0;
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
	gRequestProc = pRequestProc;

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	dirdata->gfile = filename_to_gfile(Path);

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
	GFile *gfile = filename_to_gfile(RemoteName);
	g_file_delete(gfile, NULL, NULL);
	g_object_unref(gfile);

	return TRUE;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	GFile *gfile = filename_to_gfile(RemoteName);
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

	if (CopyFlags == 0 && g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	tCopyInfo *info = g_new0(tCopyInfo, 1);

	g_strlcpy(info->in_file, RemoteName, PATH_MAX);
	g_strlcpy(info->out_file, LocalName, PATH_MAX);

	GFile *src = filename_to_gfile(RemoteName);
	GFile *dest = g_file_new_for_path(LocalName);

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
	else if (RemoteName[1] != '\0' && strcmp(RemoteName, "/..") != 0 && strcmp(Verb, "properties") == 0)
	{
		if (g_file_test(gLFMPath, G_FILE_TEST_EXISTS))
		{
			g_strlcpy(gLastFile, RemoteName, PATH_MAX);
			gDialogApi->DialogBoxLFMFile(gLFMPath, DlgProc);
		}
		else
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

	GFile *src = filename_to_gfile(split[1]);
	g_strfreev(split);

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
	g_strlcpy(ViewHeaders, _("Path"), maxlen - 1);
	g_strlcpy(ViewWidths, "100,20,150", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, _("Trash"), maxlen - 1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gDialogApi == NULL)
	{
		char langdir[PATH_MAX];
		setlocale(LC_ALL, "");
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));
		g_strlcpy(gLFMPath, gDialogApi->PluginDir, PATH_MAX);
		g_strlcpy(langdir, gDialogApi->PluginDir, PATH_MAX);
		strcat(gLFMPath, "dialog.lfm");
		strcat(langdir, "langs");
		bindtextdomain(GETTEXT_PACKAGE, langdir);
		textdomain(GETTEXT_PACKAGE);
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gDialogApi != NULL)
		g_free(gDialogApi);

	gDialogApi = NULL;
}
