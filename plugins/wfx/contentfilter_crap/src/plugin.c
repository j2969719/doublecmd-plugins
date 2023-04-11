#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox

#define ROOTNAME "Content Filter"
#define ININAME "j2969719_wfx.ini"
#define BUFSIZE 8192


typedef struct sVFSDirData
{
	DIR *cur;
	char path[PATH_MAX];
} tVFSDirData;

typedef struct sField
{
	char *name;
	int type;
} tField;

#define fieldcount (sizeof(gFields)/sizeof(tField))

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;

GKeyFile *gCfg = NULL;
static gchar *gCfgPath = NULL;
static gchar *gLastFile = NULL;
static char gStartPath[PATH_MAX] = "/";
static char gRemoteName[PATH_MAX];

gint gMode;
gboolean gCustom;
gchar **gCustomTypes = NULL;

static char gLFMPath[EXT_MAX_PATH];

tField gFields[] =
{
	{"content_type",	ft_string},
	{"description",		ft_string},
};

void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	int64_t ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

unsigned long FileTimeToUnixTime(LPFILETIME ft)
{
	int64_t ll = ft->dwHighDateTime;
	ll = (ll << 32) | ft->dwLowDateTime;
	ll = (ll - 116444736000000000) / 10000000;
	return (unsigned long)ll;
}

intptr_t DCPCALL PropertiesDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	if (Msg == DN_INITDIALOG)
	{
		GFile *gfile = g_file_new_for_path(gRemoteName);

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
					{
						str = g_strdup_printf("%s\t???", attr[i]);
					}

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

	return 0;
}

intptr_t DCPCALL OptionsDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);
		gchar *path = g_key_file_get_string(gCfg, ROOTNAME, "StartPath", NULL);

		if (path)
		{
			SendDlgMsg(pDlg, "deStartPath", DM_SETTEXT, (intptr_t)path, 0);
			g_free(path);
		}

		gMode = g_key_file_get_integer(gCfg, ROOTNAME, "Mode", NULL);
		SendDlgMsg(pDlg, "rgMode", DM_LISTSETITEMINDEX, gMode, 0);
		gCustom = g_key_file_get_boolean(gCfg, ROOTNAME, "Custom", NULL);
		SendDlgMsg(pDlg, "chCustom", DM_SETCHECK, (int)gCustom, 0);

		SendDlgMsg(pDlg, "rgMode", DM_ENABLE, (int)!gCustom, 0);
		SendDlgMsg(pDlg, "cbAvalible", DM_ENABLE, (int)gCustom, 0);
		SendDlgMsg(pDlg, "lbSelected", DM_ENABLE, (int)gCustom, 0);
		SendDlgMsg(pDlg, "btnAdd", DM_ENABLE, (int)gCustom, 0);
		SendDlgMsg(pDlg, "btnDel", DM_ENABLE, (int)gCustom, 0);


		if (gCustomTypes != NULL)
			g_strfreev(gCustomTypes);

		gsize len;
		gCustomTypes = g_key_file_get_string_list(gCfg, ROOTNAME, "CustomTypes", &len, NULL);


		if (gCustomTypes != NULL)
			for (gsize i = 0; i < len; i++)
				SendDlgMsg(pDlg, "lbSelected", DM_LISTADDSTR, (intptr_t)gCustomTypes[i], 0);


		GList *list = g_content_types_get_registered();

		for (GList *l = list; l != NULL; l = l->next)
		{
			if (gCustomTypes != NULL && g_strv_contains((const gchar * const*)gCustomTypes, l->data))
				continue;

			SendDlgMsg(pDlg, "cbAvalible", DM_LISTADDSTR, (intptr_t)l->data, 0);
		}

		g_list_free_full(list, g_free);
		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			char *path = (char*)SendDlgMsg(pDlg, "deStartPath", DM_GETTEXT, 0, 0);
			gMode = (int)SendDlgMsg(pDlg, "rgMode", DM_LISTGETITEMINDEX, 0, 0);
			gCustom = (gboolean)SendDlgMsg(pDlg, "chCustom", DM_GETCHECK, 0, 0);

			if (path && strlen(path) > 0)
			{
				g_strlcpy(gStartPath, path, sizeof(gStartPath));
				g_key_file_set_string(gCfg, ROOTNAME, "StartPath", path);
			}

			gsize count = (gsize)SendDlgMsg(pDlg, "lbSelected", DM_LISTGETCOUNT, 0, 0);

			if (gCustomTypes != NULL)
				g_strfreev(gCustomTypes);

			gCustomTypes = NULL;
			gchar *types = NULL;

			for (gsize i = 0; i < count; i++)
			{
				char *type = (char*)SendDlgMsg(pDlg, "lbSelected", DM_LISTGETITEM, i, 0);

				if (type && strlen(type) > 0)
				{
					if (!types)
						types = g_strdup_printf("%s;", type);
					else
					{
						gchar *tmp = g_strdup_printf("%s%s;", types, type);
						g_free(types);
						types = tmp;
					}
				}
			}

			if (types)
			{
				g_key_file_set_string(gCfg, ROOTNAME, "CustomTypes", types);
				g_free(types);
				gCustomTypes = g_key_file_get_string_list(gCfg, ROOTNAME, "CustomTypes", NULL, NULL);
			}
			else
				g_key_file_set_string(gCfg, ROOTNAME, "CustomTypes", "");

			g_key_file_set_integer(gCfg, ROOTNAME, "Mode", gMode);
			g_key_file_set_boolean(gCfg, ROOTNAME, "Custom", gCustom);

			g_key_file_save_to_file(gCfg, gCfgPath, NULL);
		}
		else if (strcmp(DlgItemName, "lbSelected") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "lbSelected", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				char *content = (char*)SendDlgMsg(pDlg, "lbSelected", DM_LISTGETITEM, i, 0);
				gchar *desc = g_content_type_get_description(content);
				gchar *info = g_strdup_printf("%s\n%s", content, desc);
				SendDlgMsg(pDlg, "lblInfo", DM_SETTEXT, (intptr_t)info, 0);
				g_free(desc);
				g_free(info);
			}
		}
		else if (strcmp(DlgItemName, "btnAdd") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "cbAvalible", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				gchar *content = g_strdup((char*)SendDlgMsg(pDlg, "cbAvalible", DM_LISTGETITEM, i, 0));
				SendDlgMsg(pDlg, "lbSelected", DM_LISTADDSTR, (intptr_t)content, 0);
				SendDlgMsg(pDlg, "cbAvalible", DM_LISTDELETE, i, 0);
				g_free(content);
			}
		}
		else if (strcmp(DlgItemName, "btnDel") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "lbSelected", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				gchar *content = g_strdup((char*)SendDlgMsg(pDlg, "lbSelected", DM_LISTGETITEM, i, 0));
				SendDlgMsg(pDlg, "cbAvalible", DM_LISTADDSTR, (intptr_t)content, 0);
				SendDlgMsg(pDlg, "lbSelected", DM_LISTDELETE, i, 0);
				g_free(content);
			}
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "chCustom") == 0)
		{
			gboolean state = (gboolean)SendDlgMsg(pDlg, "chCustom", DM_GETCHECK, 0, 0);
			SendDlgMsg(pDlg, "rgMode", DM_ENABLE, (int)!state, 0);
			SendDlgMsg(pDlg, "cbAvalible", DM_ENABLE, (int)state, 0);
			SendDlgMsg(pDlg, "lbSelected", DM_ENABLE, (int)state, 0);
			SendDlgMsg(pDlg, "btnAdd", DM_ENABLE, (int)state, 0);
			SendDlgMsg(pDlg, "btnDel", DM_ENABLE, (int)state, 0);
		}

		break;
	}

	return 0;
}

static BOOL PropertiesDialog(void)
{
	const char lfmdata[] = ""
	                       "object PropsDialogBox: TPropsDialogBox\n"
	                       "  Left = 458\n"
	                       "  Height = 399\n"
	                       "  Top = 307\n"
	                       "  Width = 417\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Properties'\n"
	                       "  ChildSizing.LeftRightSpacing = 10\n"
	                       "  ChildSizing.TopBottomSpacing = 10\n"
	                       "  ClientHeight = 399\n"
	                       "  ClientWidth = 417\n"
	                       "  DesignTimePPI = 100\n"
	                       "  OnShow = DialogBoxShow\n"
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object lbProps: TListBox\n"
	                       "    Left = 8\n"
	                       "    Height = 339\n"
	                       "    Top = 8\n"
	                       "    Width = 400\n"
	                       "    Columns = 2\n"
	                       "    ExtendedSelect = False\n"
	                       "    ItemHeight = 0\n"
	                       "    Options = []\n"
	                       "    TabOrder = 0\n"
	                       "  end\n"
	                       "  object btnClose: TBitBtn\n"
	                       "    AnchorSideTop.Control = lbProps\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = lbProps\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 307\n"
	                       "    Height = 31\n"
	                       "    Top = 362\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Top = 15\n"
	                       "    Cancel = True\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    Default = True\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkClose\n"
	                       "    ModalResult = 11\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 1\n"
	                       "  end\n"
	                       "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), PropertiesDlgProc);
}

static BOOL OptionsDialog(void)
{
	const char lfmdata[] = ""

;
	return gExtensions->DialogBoxLFMFile(gLFMPath, OptionsDlgProc);
	//return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), OptionsDlgProc);
}

static int CopyLocalFile(char* InFileName, char* OutFileName)
{
	int ifd, ofd, done;
	ssize_t len, total = 0;
	char buff[BUFSIZE];
	struct stat buf;
	int result = FS_FILE_OK;

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

static BOOL SetFindData(DIR *cur, char *path, WIN32_FIND_DATAA *FindData)
{
	struct dirent *ent;
	char file[PATH_MAX];

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while ((ent = readdir(cur)) != NULL)
	{
		gboolean skip = FALSE;
		snprintf(file, sizeof(file), "%s/%s", path, ent->d_name);
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		GFile *gfile = g_file_new_for_path(file);

		if (!gfile)
			continue;

		GFileInfo *fileinfo = g_file_query_info(gfile, "standard::*,time::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

		if (!gfile)
		{
			g_object_unref(gfile);
			continue;
		}

		if (g_file_info_get_file_type(fileinfo) == G_FILE_TYPE_DIRECTORY)
			FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		else
		{
			const gchar *content_type = g_file_info_get_content_type(fileinfo);

			if (gCustom && (!gCustomTypes || !g_strv_contains((const gchar* const*)gCustomTypes, content_type)))
				skip = TRUE;
			else if (gMode == 1 && strncmp(content_type, "image", 5) != 0)
				skip = TRUE;
			else if (gMode == 2 && strncmp(content_type, "audio", 5) != 0)
				skip = TRUE;
			else if (gMode == 3 && strncmp(content_type, "video", 5) != 0)
				skip = TRUE;

			if (skip)
			{
				g_object_unref(fileinfo);
				g_object_unref(gfile);
				continue;
			}
			int64_t size = (int64_t)g_file_info_get_size(fileinfo);
			FindData->nFileSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = size & 0x00000000FFFFFFFF;
		}

		int64_t timeinfo = (int64_t)g_file_info_get_attribute_uint64(fileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED);
		UnixTimeToFileTime(timeinfo, &FindData->ftLastWriteTime);
		timeinfo = (int64_t)g_file_info_get_attribute_uint64(fileinfo, G_FILE_ATTRIBUTE_TIME_ACCESS);
		UnixTimeToFileTime(timeinfo, &FindData->ftLastAccessTime);
		g_strlcpy(FindData->cFileName, ent->d_name, MAX_PATH - 1);
		g_object_unref(fileinfo);
		g_object_unref(gfile);

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

	OptionsDialog();

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	DIR *dir;
	tVFSDirData *dirdata;

	dirdata = malloc(sizeof(tVFSDirData));

	if (dirdata == NULL)
		return (HANDLE)(-1);

	memset(dirdata, 0, sizeof(tVFSDirData));
	snprintf(dirdata->path, sizeof(dirdata->path), "%s%s", gStartPath, Path);

	if ((dir = opendir(dirdata->path)) == NULL)
	{
		int errsv = errno;
		MessageBox(strerror(errsv), ROOTNAME, MB_OK | MB_ICONERROR);
		free(dirdata);
		return (HANDLE)(-1);
	}

	dirdata->cur = dir;

	if (!SetFindData(dirdata->cur, dirdata->path, FindData))
	{
		closedir(dir);
		free(dirdata);
		return (HANDLE)(-1);
	}

	return (HANDLE)dirdata;
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return SetFindData(dirdata->cur, dirdata->path, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	if (dirdata->cur != NULL)
		closedir(dirdata->cur);

	if (dirdata != NULL)
		free(dirdata);

	return 0;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	gchar *localpath = g_strdup_printf("%s%s", gStartPath, RemoteName);
	g_strlcpy(RemoteName, localpath, maxlen - 1);
	g_free(localpath);

	return TRUE;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	char realname[PATH_MAX];
	snprintf(realname, sizeof(realname), "%s%s", gStartPath, RemoteName);

	if (strcmp(LocalName, realname) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (CopyFlags == 0 && access(LocalName, F_OK) == 0)
		return FS_FILE_EXISTS;

	if (gProgressProc(gPluginNr, realname, LocalName, 0) == 1)
		return FS_FILE_USERABORT;

	return CopyLocalFile(realname, LocalName);
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	char realname[PATH_MAX];
	snprintf(realname, sizeof(realname), "%s%s", gStartPath, RemoteName);

	if (strcmp(LocalName, realname) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (CopyFlags == 0 && access(realname, F_OK) == 0)
		return FS_FILE_EXISTS;

	if (gProgressProc(gPluginNr, LocalName, realname, 0) == 1)
		return FS_FILE_USERABORT;

	return CopyLocalFile(LocalName, realname);
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct * ri)
{
	char oldpath[PATH_MAX];
	char newpath[PATH_MAX];
	snprintf(oldpath, sizeof(oldpath), "%s%s", gStartPath, OldName);
	snprintf(newpath, sizeof(newpath), "%s%s", gStartPath, NewName);

	if (!OverWrite && access(newpath, F_OK) == 0)
		return FS_FILE_EXISTS;

	if (strcmp(oldpath, newpath) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (gProgressProc(gPluginNr, oldpath, newpath, 0))
		return FS_FILE_USERABORT;

	if (!Move)
		return CopyLocalFile(oldpath, newpath);

	int result = FS_FILE_WRITEERROR;

	if (rename(oldpath, newpath) == 0)
		result = FS_FILE_OK;

	gProgressProc(gPluginNr, oldpath, newpath, 100);

	return result;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	char realname[PATH_MAX];
	snprintf(realname, sizeof(realname), "%s%s", gStartPath, Path);

	if (mkdir(realname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
		return FALSE;

	return TRUE;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	char realname[PATH_MAX];
	snprintf(realname, sizeof(realname), "%s%s", gStartPath, RemoteName);

	if (remove(realname) == -1)
		return FALSE;

	return TRUE;
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	char realname[PATH_MAX];
	snprintf(realname, sizeof(realname), "%s%s", gStartPath, RemoteName);

	if (rmdir(realname) == -1)
	{
		int errsv = errno;
		MessageBox(strerror(errsv), ROOTNAME, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	return TRUE;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	snprintf(gRemoteName, sizeof(gRemoteName), "%s%s", gStartPath, RemoteName);

	if (strcmp(Verb, "open") == 0)
	{
		gchar *quoted = g_shell_quote(gRemoteName);
		gchar *command = g_strdup_printf("xdg-open %s", quoted);
		g_free(quoted);
		g_spawn_command_line_async(command, NULL);
		g_free(command);
		return FS_FILE_OK;
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		if (strcmp(RemoteName, "/") == 0 || strcmp(RemoteName, "/..") == 0)
			OptionsDialog();
		else
			PropertiesDialog();

		return FS_EXEC_OK;
	}
	else if (strncmp(Verb, "chmod", 5) == 0)
	{
		int mode = strtoll(Verb + 6, 0, 8);

		if (chmod(gRemoteName, mode) == 0)
			return FS_EXEC_OK;
	}

	return FS_EXEC_ERROR;
}

BOOL DCPCALL FsSetTime(char* RemoteName, FILETIME *CreationTime, FILETIME *LastAccessTime, FILETIME *LastWriteTime)
{
	struct stat buf;
	struct utimbuf ubuf;

	snprintf(gRemoteName, sizeof(gRemoteName), "%s%s", gStartPath, RemoteName);

	if (LastAccessTime != NULL || LastWriteTime != NULL)
	{

		if (stat(gRemoteName, &buf) == 0)
		{
			if (LastAccessTime != NULL)
				ubuf.actime = FileTimeToUnixTime(LastAccessTime);
			else
				ubuf.actime = buf.st_atime;

			if (LastWriteTime != NULL)
				ubuf.modtime = FileTimeToUnixTime(LastWriteTime);
			else
				ubuf.modtime = buf.st_mtime;

			if (utime(gRemoteName, &ubuf) == 0)
				return TRUE;
		}
	}

	return TRUE;
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, gFields[FieldIndex].name, maxlen - 1);
	Units[0] = '\0';
	return gFields[FieldIndex].type;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int result = ft_nosuchfield;
	snprintf(gRemoteName, sizeof(gRemoteName), "%s%s", gStartPath, FileName);
	GFile *gfile = g_file_new_for_path(gRemoteName);

	if (!gfile)
		return ft_fileerror;

	GFileInfo *fileinfo = g_file_query_info(gfile, "standard::content-type", G_FILE_QUERY_INFO_NONE, NULL, NULL);

	if (!fileinfo)
	{
		g_object_unref(gfile);
		return ft_fieldempty;
	}

	const gchar *content_type = g_file_info_get_content_type(fileinfo);

	if (content_type)
	{
		if (FieldIndex == 1)
		{
			gchar *desc = g_content_type_get_description(content_type);

			if (desc)
			{
				g_strlcpy((char*)FieldValue, desc, maxlen - 1);
				result = gFields[FieldIndex].type;
				g_free(desc);
			}
			else
				result = ft_fieldempty;
		}
		else if (FieldIndex == 0)
		{
			g_strlcpy((char*)FieldValue, content_type, maxlen - 1);
			result = gFields[FieldIndex].type;
		}
	}
	else
		result = ft_fieldempty;

	g_object_unref(fileinfo);
	g_object_unref(gfile);

	return result;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	g_strlcpy(ViewHeaders, "Content Type\\nDescription", maxlen - 1);
	g_strlcpy(ViewWidths, "100,20,40,80", maxlen - 1);
	g_strlcpy(ViewContents, "[Plugin(FS).content_type{}]\\n[Plugin(FS).description{}]", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);

	return TRUE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	snprintf(DefRootName, maxlen - 1, ROOTNAME);
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	if (gCfg == NULL)
	{
		gCfg = g_key_file_new();
		gchar *cfgdir = g_path_get_dirname(dps->DefaultIniName);
		gCfgPath = g_strdup_printf("%s/%s", cfgdir, ININAME);
		g_free(cfgdir);
	}
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
		snprintf(gLFMPath, sizeof(gLFMPath), "%sdialog.lfm", gExtensions->PluginDir);
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
		free(gExtensions);

	gExtensions = NULL;

	if (gCfg != NULL)
	{
		g_key_file_free(gCfg);
		g_free(gCfgPath);
	}

	gCfg = NULL;

	g_free(gLastFile);
	gLastFile = NULL;

	if (gCustomTypes != NULL)
		g_strfreev(gCustomTypes);

	gCustomTypes = NULL;
}
