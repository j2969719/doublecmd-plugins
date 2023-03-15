#define _GNU_SOURCE
#include <glib.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <magic.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
#include <fnmatch.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox

#define ROOTNAME "Match Pattern"
#define ININAME "j2969719_wfx.ini"
#define MAXINIITEMS 256
#define BUFSIZE 8192


typedef struct sVFSDirData
{
	DIR *cur;
	char path[PATH_MAX];
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;

GKeyFile *gCfg = NULL;
static gchar *gCfgPath = NULL;
int gMatchFlags = 0;
static char gPattern[256] = "*";
static char gStartPath[PATH_MAX] = "/";
static char gRemoteName[PATH_MAX];


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

intptr_t DCPCALL FilterDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		SendDlgMsg(pDlg, "deStratPath", DM_SETTEXT, (intptr_t)gStartPath, 0);
		SendDlgMsg(pDlg, "cbPattern", DM_SETTEXT, (intptr_t)gPattern, 0);
		g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);

		gchar **items = g_key_file_get_keys(gCfg, ROOTNAME, NULL, NULL);

		if (items)
			for (gsize i = 0; items[i] != NULL; i++)
			{
				if (strncmp(items[i], "Flags", 5) == 0)
				{
					gMatchFlags = g_key_file_get_integer(gCfg, ROOTNAME, items[i], NULL);
				}
				else if (strncmp(items[i], "Pattern", 4) == 0)
				{
					gchar *item = g_key_file_get_string(gCfg, ROOTNAME, items[i], NULL);

					if (item && strlen(item) > 0)
						SendDlgMsg(pDlg, "cbPattern", DM_LISTADDSTR, (intptr_t)item, 0);

					g_free(item);
				}
			}

		g_strfreev(items);

		if (gMatchFlags & FNM_CASEFOLD)
			SendDlgMsg(pDlg, "chCaseFold", DM_SETCHECK, 1, 0);

		if (gMatchFlags & FNM_NOESCAPE)
			SendDlgMsg(pDlg, "cbNoescape", DM_SETCHECK, 1, 0);

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			g_strlcpy(gStartPath, (char*)SendDlgMsg(pDlg, "deStratPath", DM_GETTEXT, 0, 0), sizeof(gPattern));

			int len = strlen(gStartPath);

			if (len > 1 && gStartPath[len - 1] == '/')
				gStartPath[len - 1] = '\0';

			g_strlcpy(gPattern, (char*)SendDlgMsg(pDlg, "cbPattern", DM_GETTEXT, 0, 0), sizeof(gPattern));

			gMatchFlags = 0;

			if (SendDlgMsg(pDlg, "chCaseFold", DM_GETCHECK, 0, 0))
				gMatchFlags |= FNM_CASEFOLD;

			if (SendDlgMsg(pDlg, "cbNoescape", DM_GETCHECK, 0, 0))
				gMatchFlags |= FNM_NOESCAPE;

			g_key_file_set_string(gCfg, ROOTNAME, "Path", gStartPath);
			g_key_file_set_string(gCfg, ROOTNAME, "Pattern0", gPattern);
			g_key_file_set_integer(gCfg, ROOTNAME, "Flags", gMatchFlags);

			gsize count = (gsize)SendDlgMsg(pDlg, "cbPattern", DM_LISTGETCOUNT, 0, 0);
			int num = 1;

			for (gsize i = 0; i < count; i++)
			{
				char *line = (char*)SendDlgMsg(pDlg, "cbPattern", DM_LISTGETITEM, i, 0);

				if (g_strcmp0(line, gPattern) != 0)
				{
					gchar *key = g_strdup_printf("Pattern%d", num++);
					g_key_file_set_string(gCfg, ROOTNAME, key, line);
					g_free(key);
				}

				if (num == MAXINIITEMS)
					break;
			}

			g_key_file_save_to_file(gCfg, gCfgPath, NULL);

		}

		break;
	}

	return 0;
}


intptr_t DCPCALL PropertiesDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	if (Msg == DN_INITDIALOG)
	{
		char text[400];
		struct stat buf;
		const char *magic_full;
		magic_t magic_cookie = magic_open(MAGIC_NONE);

		if (magic_cookie)
		{
			if (magic_load(magic_cookie, NULL) == 0)
			{
				magic_full = magic_file(magic_cookie, gRemoteName);
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)magic_full, 0);
			}

			magic_close(magic_cookie);
		}

		if (lstat(gRemoteName, &buf) == 0)
		{
			snprintf(text, sizeof(text), "size\t%ld",  buf.st_size);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "inode\t%lu",  buf.st_ino);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "blksize\t%ld",  buf.st_blksize);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "blocks\t%ld",  buf.st_blocks);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "nlink\t%lu",  buf.st_nlink);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "mtime\t%s",  ctime(&buf.st_mtime));
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "atime\t%s",  ctime(&buf.st_atime));
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "ctime\t%s",  ctime(&buf.st_ctime));
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "mode\t%o",  buf.st_mode);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
		}

		struct passwd *pw = getpwuid(buf.st_uid);

		if (pw)
		{
			snprintf(text, sizeof(text), "owner\t%s (%d)",  pw->pw_name, buf.st_uid);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
		}

		struct group  *gr = getgrgid(buf.st_gid);

		if (gr)
		{
			snprintf(text, sizeof(text), "group\t%s (%d)",  gr->gr_name, buf.st_gid);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
		}

		char list[XATTR_SIZE_MAX];
		ssize_t len = listxattr(gRemoteName, list, XATTR_SIZE_MAX);

		if (len > 0)
		{
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"xattrs", 0);

			for (int i = 0; i < len; i += strlen(&list[i]) + 1)
			{
				char value[XATTR_SIZE_MAX];
				ssize_t vlen = getxattr(gRemoteName, &list[i], value, XATTR_SIZE_MAX);

				if (vlen > -1)
				{
					snprintf(text, sizeof(text), "%s\t%s",  &list[i], value);
					SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
				}
			}
		}


	}

	return 0;
}

static BOOL FilterDialog(void)
{
	const char lfmdata[] = ""
	                       "object DialogBox: TDialogBox\n"
	                       "  Left = 458\n"
	                       "  Height = 190\n"
	                       "  Top = 307\n"
	                       "  Width = 598\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Match Pattern'\n"
	                       "  ChildSizing.LeftRightSpacing = 10\n"
	                       "  ChildSizing.TopBottomSpacing = 10\n"
	                       "  ClientHeight = 190\n"
	                       "  ClientWidth = 598\n"
	                       "  DesignTimePPI = 100\n"
	                       "  OnShow = DialogBoxShow\n"
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object lblStartPath: TLabel\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = deStratPath\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    Left = 10\n"
	                       "    Height = 17\n"
	                       "    Top = 13\n"
	                       "    Width = 31\n"
	                       "    BorderSpacing.Right = 5\n"
	                       "    Caption = 'Path'\n"
	                       "    ParentColor = False\n"
	                       "  end\n"
	                       "  object deStratPath: TDirectoryEdit\n"
	                       "    AnchorSideLeft.Control = lblStartPath\n"
	                       "    AnchorSideLeft.Side = asrBottom\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    AnchorSideRight.Control = cbNoescape\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 46\n"
	                       "    Height = 23\n"
	                       "    Top = 10\n"
	                       "    Width = 516\n"
	                       "    Directory = '/'\n"
	                       "    ShowHidden = False\n"
	                       "    ButtonWidth = 24\n"
	                       "    NumGlyphs = 1\n"
	                       "    Flat = True\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    BorderSpacing.Left = 5\n"
	                       "    BorderSpacing.Top = 5\n"
	                       "    MaxLength = 0\n"
	                       "    TabOrder = 0\n"
	                       "    Text = '/'\n"
	                       "  end\n"
	                       "  object lblPattern: TLabel\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = cbPattern\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    Left = 10\n"
	                       "    Height = 17\n"
	                       "    Top = 41\n"
	                       "    Width = 87\n"
	                       "    BorderSpacing.Right = 5\n"
	                       "    Caption = 'Unix Pattern'\n"
	                       "    ParentColor = False\n"
	                       "  end\n"
	                       "  object cbPattern: TComboBox\n"
	                       "    AnchorSideLeft.Control = lblPattern\n"
	                       "    AnchorSideLeft.Side = asrBottom\n"
	                       "    AnchorSideTop.Control = deStratPath\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 102\n"
	                       "    Height = 23\n"
	                       "    Top = 38\n"
	                       "    Width = 250\n"
	                       "    BorderSpacing.Left = 5\n"
	                       "    BorderSpacing.Top = 5\n"
	                       "    ItemHeight = 17\n"
	                       "    MaxLength = 256\n"
	                       "    TabOrder = 1\n"
	                       "    Text = '*'\n"
	                       "  end\n"
	                       "  object cbNoescape: TCheckBox\n"
	                       "    AnchorSideLeft.Control = chCaseFold\n"
	                       "    AnchorSideLeft.Side = asrBottom\n"
	                       "    AnchorSideTop.Control = cbPattern\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    Left = 464\n"
	                       "    Height = 22\n"
	                       "    Top = 38\n"
	                       "    Width = 98\n"
	                       "    BorderSpacing.Left = 10\n"
	                       "    Caption = 'No Escape'\n"
	                       "    TabOrder = 3\n"
	                       "  end\n"
	                       "  object chCaseFold: TCheckBox\n"
	                       "    AnchorSideLeft.Control = cbPattern\n"
	                       "    AnchorSideLeft.Side = asrBottom\n"
	                       "    AnchorSideTop.Control = cbPattern\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    Left = 362\n"
	                       "    Height = 22\n"
	                       "    Top = 38\n"
	                       "    Width = 92\n"
	                       "    BorderSpacing.Left = 10\n"
	                       "    Caption = 'Case Fold'\n"
	                       "    TabOrder = 2\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = cbPattern\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = deStratPath\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 461\n"
	                       "    Height = 31\n"
	                       "    Top = 76\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Top = 15\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    Default = True\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkOK\n"
	                       "    ModalResult = 1\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 4\n"
	                       "  end\n"
	                       "  object btnCancel: TBitBtn\n"
	                       "    AnchorSideTop.Control = btnOK\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    Left = 355\n"
	                       "    Height = 31\n"
	                       "    Top = 76\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Right = 5\n"
	                       "    Cancel = True\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkCancel\n"
	                       "    ModalResult = 2\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 5\n"
	                       "  end\n"
	                       "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), FilterDlgProc);
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
	struct stat buf;
	char file[PATH_MAX];

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while ((ent = readdir(cur)) != NULL)
	{

		snprintf(file, sizeof(file), "%s/%s", path, ent->d_name);
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;

		if (stat(file, &buf) == 0)
		{
			if (S_ISDIR(buf.st_mode))
				FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			else
			{
				if (fnmatch(gPattern, ent->d_name, gMatchFlags) != 0)
					continue;

				FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
				FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;
			}

			UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
			UnixTimeToFileTime(buf.st_atime, &FindData->ftLastAccessTime);
			FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
			FindData->dwReserved0 = buf.st_mode;
		}
		else
			continue;

		g_strlcpy(FindData->cFileName, ent->d_name, MAX_PATH - 1);

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

	FilterDialog();

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

	// iwanttobelive
	gboolean wtf_overwrite = (gboolean)abs((int)OverWrite % 2);

	if (!wtf_overwrite && access(newpath, F_OK) == 0)
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
	int result = FS_EXEC_ERROR;

	snprintf(gRemoteName, sizeof(gRemoteName), "%s%s", gStartPath, RemoteName);

	if (strcmp(Verb, "open") == 0)
	{
		struct stat buf;

		if (stat(gRemoteName, &buf) == 0)
		{
			char *command = malloc(strlen(gRemoteName) + 13);

			if (buf.st_mode & S_IXUSR)
				sprintf(command, "\"%s\"", gRemoteName);
			else
				sprintf(command, "xdg-open \"%s\"", gRemoteName);

			system(command);
			free(command);
			result = FS_EXEC_OK;
		}
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		if (strcmp(RemoteName, "/") == 0 || strcmp(RemoteName, "/..") == 0)
		{
			FilterDialog();
			result = FS_EXEC_OK;
		}
		else if (PropertiesDialog())
			result = FS_EXEC_OK;
	}
	else if (strncmp(Verb, "chmod", 5) == 0)
	{
		int mode = strtoll(Verb + 6, 0, 8);

		if (chmod(gRemoteName, mode) == 0)
			return FS_EXEC_OK;
	}

	return result;
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
		g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);
		g_free(cfgdir);
		gchar *item = g_key_file_get_string(gCfg, ROOTNAME, "Path", NULL);

		if (item)
		{
			g_strlcpy(gStartPath, item, sizeof(gStartPath));
			g_free(item);
		}

		item = g_key_file_get_string(gCfg, ROOTNAME, "Pattern0", NULL);

		if (item)
		{
			g_strlcpy(gPattern, item, sizeof(gPattern));
			g_free(item);
		}

		gMatchFlags = g_key_file_get_integer(gCfg, ROOTNAME, "Flags", NULL);
	}
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

	if (gCfg != NULL)
	{
		g_key_file_free(gCfg);
		g_free(gCfgPath);
	}

	gCfg = NULL;
}
