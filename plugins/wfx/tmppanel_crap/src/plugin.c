#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define BUFSIZE 8192
#define ROOTNAME "TmpPanel"
#define MessageBox gExtensions->MessageBox
#define SendDlgMsg gExtensions->SendDlgMsg

typedef struct sVFSDirData
{
	gchar **files;
	gchar group[PATH_MAX];
	gsize ifile;
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;

GKeyFile *gCfg = NULL;
gchar *gCfgPath = NULL;

tExtensionStartupInfo* gExtensions = NULL;
static char gLFMPath[EXT_MAX_PATH];

static char gGroup[MAX_PATH];
static char gDisplayName[MAX_PATH];
static char gRemoteName[PATH_MAX];

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

static int CopyLocalFile(char* InFileName, char* OutFileName)
{
	int ifd, ofd, done;
	ssize_t len, total = 0;
	char buff[BUFSIZE];
	struct stat buf;
	int result = FS_FILE_OK;

	if (strcmp(InFileName, OutFileName) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (stat(InFileName, &buf) != 0)
		return FS_FILE_READERROR;

	ifd = open(InFileName, O_RDONLY);

	if (ifd == -1)
		return FS_FILE_READERROR;

	ofd = open(OutFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (ofd > -1)
	{

		while ((len = read(ifd, buff, sizeof(buff))) > 0)
		{
			if (write(ofd, buff, len) == -1)
			{
				result = FS_FILE_WRITEERROR;
				break;
			}

			total += len;

			if (buf.st_size > 0)
				done = total * 100 / buf.st_size;
			else
				done = 0;

			if (done > 100)
				done = 100;

			if (gProgressProc(gPluginNr, InFileName, OutFileName, done) == 1)
			{
				result = FS_FILE_USERABORT;
				remove(OutFileName);
				break;
			}
		}

		close(ofd);
		chmod(OutFileName, buf.st_mode);
	}
	else
		result = FS_FILE_WRITEERROR;

	close(ifd);

	return result;
}

static void FillProps(char* FileName, uintptr_t pDlg)
{
	SendDlgMsg(pDlg, "lbProps", DM_LISTCLEAR, 0, 0);
	GFile *gfile = g_file_new_for_path(FileName);

	if (gfile)
	{
		GFileInfo *fileinfo = g_file_query_info(gfile, "*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

		if (fileinfo)
		{
			gchar **attr = g_file_info_list_attributes(fileinfo, NULL);
			guint len = g_strv_length(attr);

			for (guint i = 0; i < len; i++)
			{
				gchar *str = NULL;
				GFileAttributeType type = g_file_info_get_attribute_type(fileinfo, attr[i]);

				if (type == G_FILE_ATTRIBUTE_TYPE_STRING)
				{
					const char * val = g_file_info_get_attribute_string(fileinfo, attr[i]);
					str = g_strdup_printf("%s\t%s", attr[i], val);
				}
				else if (type == G_FILE_ATTRIBUTE_TYPE_INT32)
				{
					gint32 val = g_file_info_get_attribute_int32(fileinfo, attr[i]);
					str = g_strdup_printf("%s\t%d", attr[i], val);
				}
				else if (type == G_FILE_ATTRIBUTE_TYPE_UINT32)
				{
					guint32 val = g_file_info_get_attribute_uint32(fileinfo, attr[i]);

					if (g_strcmp0(attr[i], "unix::mode") == 0)
						str = g_strdup_printf("%s\t%o", attr[i], val);
					else
						str = g_strdup_printf("%s\t%u", attr[i], val);
				}
				else if (type == G_FILE_ATTRIBUTE_TYPE_UINT64)
				{
					guint64 val = g_file_info_get_attribute_uint64(fileinfo, attr[i]);

					if (g_str_has_prefix(attr[i], "time::") && !g_str_has_suffix(attr[i], "sec"))
					{
						time_t tval = (time_t)val;
						str = g_strdup_printf("%s\t%s", attr[i], ctime(&tval));
						int len = strlen(str);

						if (len > 1 && str[len - 1] == '\n')
							str[len - 1] = '\0';
					}
					else
						str = g_strdup_printf("%s\t%lu", attr[i], val);
				}
				else if (type == G_FILE_ATTRIBUTE_TYPE_INT64)
				{
					gint64 val = g_file_info_get_attribute_int64(fileinfo, attr[i]);
					str = g_strdup_printf("%s\t%ld", attr[i], val);
				}
				else if (type == G_FILE_ATTRIBUTE_TYPE_BOOLEAN)
				{
					gboolean val = g_file_info_get_attribute_boolean(fileinfo, attr[i]);
					str = g_strdup_printf("%s\t%s", attr[i], val ? "true" : "false");
				}
				else
					continue;

				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)str, 0);
				g_free(str);
			}

			g_strfreev(attr);
			g_object_unref(fileinfo);
		}
		else
			MessageBox("Failed to query fileinfo", ROOTNAME, MB_OK | MB_ICONERROR);

		g_object_unref(gfile);
	}
	else
		MessageBox("Failed to open gfile", ROOTNAME, MB_OK | MB_ICONERROR);
}

intptr_t DCPCALL PropertiesDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	if (Msg == DN_INITDIALOG)
	{
		FillProps(gRemoteName, pDlg);
		SendDlgMsg(pDlg, "lblName", DM_SETTEXT, (intptr_t)gDisplayName, 0);
		SendDlgMsg(pDlg, "fnRealPath", DM_SETTEXT, (intptr_t)gRemoteName, 0);
	}
	else if (Msg == DN_CLICK && strcmp(DlgItemName, "btnApply") == 0)
	{
		gchar *newpath = g_strdup((char*)SendDlgMsg(pDlg, "fnRealPath", DM_GETTEXT, 0, 0));
		g_key_file_set_string(gCfg, gGroup, gDisplayName, newpath);
		FillProps(newpath, pDlg);
		g_free(newpath);
	}

	return 0;
}

static BOOL PropertiesDialog(void)
{
	const char lfmdata[] = ""
	                       "object DialogBox: TDialogBox\n"
	                       "  Left = 863\n"
	                       "  Height = 373\n"
	                       "  Top = 254\n"
	                       "  Width = 507\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Properties'\n"
	                       "  ChildSizing.LeftRightSpacing = 10\n"
	                       "  ChildSizing.TopBottomSpacing = 10\n"
	                       "  ChildSizing.HorizontalSpacing = 10\n"
	                       "  ChildSizing.VerticalSpacing = 10\n"
	                       "  ClientHeight = 373\n"
	                       "  ClientWidth = 507\n"
	                       "  DesignTimePPI = 100\n"
	                       "  OnShow = DialogBoxShow\n"
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object lblName: TLabel\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    AnchorSideRight.Control = lbProps\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 1\n"
	                       "    Top = 20\n"
	                       "    Width = 464\n"
	                       "    Align = alCustom\n"
	                       "    Alignment = taCenter\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    BorderSpacing.Top = 20\n"
	                       "    Font.Style = [fsBold]\n"
	                       "    ParentColor = False\n"
	                       "    ParentFont = False\n"
	                       "  end\n"
	                       "  object lbProps: TListBox\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = lblName\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = btnApply\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 219\n"
	                       "    Top = 41\n"
	                       "    Width = 464\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    BorderSpacing.Top = 20\n"
	                       "    ClickOnSelChange = False\n"
	                       "    ExtendedSelect = False\n"
	                       "    ItemHeight = 0\n"
	                       "    Options = []\n"
	                       "    TabOrder = 0\n"
	                       "  end\n"
	                       "  object fnRealPath: TFileNameEdit\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = lbProps\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 36\n"
	                       "    Top = 270\n"
	                       "    Width = 398\n"
	                       "    Filter = 'All Files|*'\n"
	                       "    FilterIndex = 0\n"
	                       "    HideDirectories = False\n"
	                       "    ButtonWidth = 24\n"
	                       "    NumGlyphs = 1\n"
	                       "    MaxLength = 0\n"
	                       "    TabOrder = 1\n"
	                       "  end\n"
	                       "  object btnApply: TBitBtn\n"
	                       "    AnchorSideLeft.Control = fnRealPath\n"
	                       "    AnchorSideLeft.Side = asrBottom\n"
	                       "    AnchorSideTop.Control = fnRealPath\n"
	                       "    AnchorSideBottom.Control = fnRealPath\n"
	                       "    AnchorSideBottom.Side = asrBottom\n"
	                       "    Left = 418\n"
	                       "    Height = 36\n"
	                       "    Top = 270\n"
	                       "    Width = 56\n"
	                       "    Anchors = [akTop, akLeft, akBottom]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Left = 5\n"
	                       "    BorderSpacing.Right = 5\n"
	                       "    OnClick = ButtonClick\n"
	                       "    Caption = 'Apply'\n"
	                       "    TabOrder = 2\n"
	                       "  end\n"
	                       "  object btnClose: TBitBtn\n"
	                       "    AnchorSideTop.Control = fnRealPath\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = lbProps\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 373\n"
	                       "    Height = 31\n"
	                       "    Top = 321\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    BorderSpacing.Top = 15\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    Default = True\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkClose\n"
	                       "    ModalResult = 11\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 3\n"
	                       "  end\n"
	                       "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), PropertiesDlgProc);
}

gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	struct stat buf;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (!dirdata->files)
		return FALSE;

	char *file = dirdata->files[dirdata->ifile];

	if (file != NULL)
	{
		g_strlcpy(FindData->cFileName, file, MAX_PATH - 1);
		gchar *target = g_key_file_get_string(gCfg, dirdata->group, file, NULL);

		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;

		if ((target) && (strncmp(target, "folder", 6) == 0))
		{
			//SetCurrentFileTime(&FindData->ftLastWriteTime);
			FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_UNIX_MODE;
			FindData->dwReserved0 = 16877;
		}
		else
		{
			if ((lstat(target, &buf) == 0))
			{
				FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
				FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;
				FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
				FindData->dwReserved0 = buf.st_mode;
				UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
				UnixTimeToFileTime(buf.st_atime, &FindData->ftLastAccessTime);
			}
			else
			{
				FindData->nFileSizeHigh = 0xFFFFFFFF;
				FindData->nFileSizeLow = 0xFFFFFFFE;
				//SetCurrentFileTime(&FindData->ftLastWriteTime);
			}
		}

		dirdata->ifile++;
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

	dirdata->ifile = 0;
	g_strlcpy(dirdata->group, Path, PATH_MAX);

	if (dirdata->group[1] != '\0' && dirdata->group[(strlen(dirdata->group) - 1)] == '/')
		dirdata->group[(strlen(dirdata->group) - 1)] = '\0';

	dirdata->files = g_key_file_get_keys(gCfg, dirdata->group, NULL, NULL);

	if (dirdata->files != NULL && SetFindData(dirdata, FindData))
		return (HANDLE)dirdata;

	if (dirdata->files != NULL)
		g_strfreev(dirdata->files);

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


	if (dirdata->files != NULL)
		g_strfreev(dirdata->files);

	g_free(dirdata);

	return 0;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);
	gchar *result = g_key_file_get_string(gCfg, group, key, NULL);
	g_free(group);
	g_free(key);

	if (result)
	{
		g_strlcpy(RemoteName, result, maxlen - 1);
		g_free(result);
		return TRUE;
	}
	else
		return FALSE;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int result = FS_FILE_OK;
	GError *err = NULL;

	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0))
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);
	gchar *realname = g_key_file_get_string(gCfg, group, key, NULL);
	g_free(group);
	g_free(key);

	if (realname)
	{
		if (g_strcmp0(realname, LocalName) == 0)
			result = FS_FILE_NOTSUPPORTED;
		else
			result = CopyLocalFile(realname, LocalName);

		g_free(realname);
	}
	else
		result = FS_FILE_READERROR;

	return result;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	gboolean result = FALSE;
	gchar *group = g_path_get_dirname(Path);
	gchar *key = g_path_get_basename(Path);

	if (!g_key_file_has_key(gCfg, group, key, NULL))
	{
		g_key_file_set_string(gCfg, group, key, "folder");
		result = TRUE;
	}

	g_free(group);
	g_free(key);

	return result;
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	gboolean result = FALSE;
	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);

	if (g_key_file_remove_key(gCfg, group, key, NULL))
	{
		g_key_file_remove_group(gCfg, RemoteName, NULL);
		result = TRUE;
	}

	g_free(group);
	g_free(key);

	return result;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	gboolean result = FALSE;
	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);

	if (g_key_file_remove_key(gCfg, group, key, NULL))
		result = TRUE;

	g_free(group);
	g_free(key);

	return result;

}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);

	if (err)
		return FS_FILE_USERABORT;

	int result = FS_FILE_OK;
	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);
	gchar *value = g_key_file_get_string(gCfg, group, key, NULL);

	if ((CopyFlags == 0) && (g_key_file_has_key(gCfg, group, key, NULL)))
		result = FS_FILE_EXISTS;
	else if (value && strncmp(value, "folder", 6) == 0)
		result = FS_FILE_WRITEERROR;
	else
		g_key_file_set_string(gCfg, group, key, LocalName);

	g_free(group);
	g_free(key);
	g_free(value);
	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	return result;
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct* ri)
{
	int err = gProgressProc(gPluginNr, OldName, NewName, 0);

	if (err)
		return FS_FILE_USERABORT;

	int result = FS_FILE_OK;
	gchar *group = g_path_get_dirname(OldName);
	gchar *key = g_path_get_basename(OldName);
	gchar *value = g_key_file_get_string(gCfg, group, key, NULL);
	gchar *newgroup = g_path_get_dirname(NewName);
	gchar *newkey = g_path_get_basename(NewName);

	// iwanttobelive
	gboolean wtf_overwrite = (gboolean)abs((int)OverWrite % 2);

	if (!wtf_overwrite && g_key_file_has_key(gCfg, newgroup, newkey, NULL))
		result = FS_FILE_EXISTS;
	else if (value && strncmp(value, "folder", 6) == 0)
		result = FS_FILE_WRITEERROR;
	else
	{
		g_key_file_set_string(gCfg, newgroup, newkey, value);
		gProgressProc(gPluginNr, OldName, NewName, 50);

		if (Move)
			g_key_file_remove_key(gCfg, group, key, NULL);
	}

	g_free(group);
	g_free(key);
	g_free(value);
	g_free(newgroup);
	g_free(newkey);
	gProgressProc(gPluginNr, OldName, NewName, 100);

	return result;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	struct stat buf;
	gchar *command = NULL;
	int result = FS_EXEC_ERROR;
	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);
	gchar *path = g_key_file_get_string(gCfg, group, key, NULL);

	if (strncmp(Verb, "open", 5) == 0)
	{
		if (path && stat(path, &buf) == 0)
		{
			if (buf.st_mode & S_IXUSR)
				command = g_shell_quote(path);
			else
			{
				gchar *quoted = g_shell_quote(path);
				command = g_strdup_printf("xdg-open %s", quoted);
				g_free(quoted);
			}

			g_spawn_command_line_async(command, NULL);
			g_free(command);
			result = FS_FILE_OK;
		}
	}
	else if (path && strncmp(path, "folder", 6) != 0 && strncmp(Verb, "chmod", 5) == 0)
	{
		gint i = g_ascii_strtoll(Verb + 6, 0, 8);

		if (chmod(path, i) == -1)
		{
			int errsv = errno;
			gRequestProc(gPluginNr, RT_MsgOK, NULL, strerror(errsv), NULL, 0);
		}

		result = FS_FILE_OK;
	}
	else if (path && path[1] != '\0' && strncmp(path, "folder", 6) != 0 && strcmp(Verb, "properties") == 0)
	{
		g_strlcpy(gRemoteName, path, sizeof(gRemoteName));
		g_strlcpy(gDisplayName, key, sizeof(gDisplayName));
		g_strlcpy(gGroup, group, sizeof(gGroup));
		PropertiesDialog();
		result = FS_FILE_OK;
	}
	else
		MessageBox(strerror(EOPNOTSUPP), ROOTNAME, MB_OK | MB_ICONERROR);

	g_free(key);
	g_free(path);
	g_free(group);

	return result;
}

BOOL DCPCALL FsSetTime(char* RemoteName, FILETIME *CreationTime, FILETIME *LastAccessTime, FILETIME *LastWriteTime)
{
	struct stat buf;
	struct utimbuf ubuf;
	gboolean result = FALSE;

	if (LastAccessTime != NULL || LastWriteTime != NULL)
	{

		gchar *group = g_path_get_dirname(RemoteName);
		gchar *key = g_path_get_basename(RemoteName);
		gchar *value = g_key_file_get_string(gCfg, group, key, NULL);

		if (value && strncmp(value, "folder", 6) != 0 && g_stat(value, &buf) == 0)
		{
			if (LastAccessTime != NULL)
				ubuf.actime = FileTimeToUnixTime(LastAccessTime);
			else
				ubuf.actime = buf.st_atime;

			if (LastWriteTime != NULL)
				ubuf.modtime = FileTimeToUnixTime(LastWriteTime);
			else
				ubuf.modtime = buf.st_mtime;

			if (utime(value, &ubuf) == 0)
				result = TRUE;
		}

		g_free(group);
		g_free(key);
		g_free(value);
	}


	return result;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	return FALSE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, ROOTNAME, maxlen - 1);
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	GError *err = NULL;
	const gchar *inifile = "tmppanel_crap.ini";

	gchar *cfg_dir = g_path_get_dirname(dps->DefaultIniName);
	gCfgPath = g_strdup_printf("%s/tmppanel_crap.ini", cfg_dir);
	g_free(cfg_dir);

	gCfg = g_key_file_new();

	if (!g_key_file_load_from_file(gCfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("(%s): %s\n", gCfgPath, (err)->message);

	if (err)
		g_error_free(err);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
		free(gExtensions);

	gExtensions = NULL;

	g_key_file_save_to_file(gCfg, gCfgPath, NULL);

	if (gCfg)
		g_key_file_free(gCfg);

	g_free(gCfgPath);
}
