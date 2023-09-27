#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <utime.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox

#define FALSE 0
#define TRUE !FALSE

#define ROOTNAME "Case-insensitive Copy"
#define HISTNAME "history_" PLUGNAME ".txt"
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

static char gStartPath[PATH_MAX];
static char gHistoryFile[PATH_MAX];
static char gRemoteName[PATH_MAX];
static char gCaseName[PATH_MAX];
BOOL gFileListOP = FALSE;

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

static int CaseDuplExists(char* FileName, BOOL SkipSelf)
{
	gCaseName[0] = '\0';
	char file[PATH_MAX];
	snprintf(file, PATH_MAX, "%s", FileName);
	char *filename = basename(file);
	char out[PATH_MAX];
	snprintf(out, PATH_MAX, "%s", FileName);
	char *outdir = dirname(out);
	DIR *dir = opendir(outdir);

	if (dir)
	{
		struct dirent *ent;

		while ((ent = readdir(dir)) != NULL)
		{
			if (strcasecmp(filename, ent->d_name) == 0)
			{
				if (SkipSelf != FALSE && strcmp(filename, ent->d_name) == 0)
					continue;

				snprintf(gCaseName, PATH_MAX, "%s", ent->d_name);
				closedir(dir);
				return FS_FILE_EXISTS;
			}
		}

		closedir(dir);
	}

	return FS_FILE_OK;
}

static void BuildCasePath(char *RemoteName, char *Path)
{
	char *pos;
	char localpath[PATH_MAX];
	char casepath[PATH_MAX] = "";
	char remote[PATH_MAX];
	snprintf(remote, PATH_MAX, "%s", RemoteName + 1);
	char *backup = strdup(gCaseName);

	while ((pos = strchr(remote, '/')) != NULL)
	{
		*pos = '\0';
		snprintf(Path, PATH_MAX, "%s%s%s/%s", (gStartPath[0] != '\0') ? gStartPath : "/", (casepath[0] != '/') ? "/" : "", (casepath[0] == '\0') ? "" : casepath, remote);
		snprintf(localpath, PATH_MAX, "%s", casepath);

		if (CaseDuplExists(Path, TRUE) == FS_FILE_EXISTS)
			snprintf(casepath, PATH_MAX, "%s/%s", localpath, gCaseName);
		else
			snprintf(casepath, PATH_MAX, "%s/%s", localpath, remote);

		snprintf(remote, PATH_MAX, "%s", pos + 1);
	}

	snprintf(gCaseName, PATH_MAX, "%s", backup);
	free(backup);
	snprintf(Path, PATH_MAX, "%s%s/%s", gStartPath, casepath, remote);
}

static int CopyLocalFile(char* InFileName, char* OutFileName)
{
	int ifd, ofd, done;
	ssize_t len, total = 0;
	char buff[BUFSIZE];
	struct stat buf;
	int result = FS_FILE_OK;
	char file[PATH_MAX];

	snprintf(file, PATH_MAX, "%s", OutFileName);

	if (gCaseName[0] != '\0')
	{
		char *pos = strrchr(file, '/');

		if (pos)
			strcpy(pos + 1, gCaseName);
	}

	if (strcmp(InFileName, file) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (stat(InFileName, &buf) != 0)
		return FS_FILE_READERROR;

	ifd = open(InFileName, O_RDONLY);

	if (ifd == -1)
		return FS_FILE_READERROR;

	ofd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

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

			if (gProgressProc(gPluginNr, InFileName, file, done) == 1)
			{
				result = FS_FILE_USERABORT;
				remove(OutFileName);
				break;
			}
		}

		close(ofd);
		chmod(file, buf.st_mode);
		struct utimbuf ubuf;
		ubuf.actime = buf.st_atime;
		ubuf.modtime = buf.st_mtime;
		utime(file, &ubuf);

		char list[XATTR_SIZE_MAX];
		ssize_t len = listxattr(InFileName, list, XATTR_SIZE_MAX);

		if (len > 0)
		{
			for (int i = 0; i < len; i += strlen(&list[i]) + 1)
			{
				char value[XATTR_SIZE_MAX];
				ssize_t value_len = getxattr(InFileName, &list[i], value, XATTR_SIZE_MAX);

				if (value_len > -1)
					setxattr(file,  &list[i], value, value_len, XATTR_CREATE);
			}
		}
	}
	else
		result = FS_FILE_WRITEERROR;

	close(ifd);

	return result;
}

intptr_t DCPCALL PropertiesDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	if (Msg == DN_INITDIALOG)
	{
		char text[400];
		struct statx buf;
		char res_path[PATH_MAX];
		char mode[] = "----------";

		SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)gRemoteName, 0);

		if (realpath(gRemoteName, res_path) && strcmp(gRemoteName, res_path) != 0)
		{
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"realpath:", 0);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)res_path, 0);
		}

		if (statx(AT_FDCWD, gRemoteName, AT_SYMLINK_NOFOLLOW, STATX_ALL, &buf) == 0)
		{
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, 0, 0);

			if (S_ISREG(buf.stx_mode))
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"type\tregular file", 0);
			else if (S_ISDIR(buf.stx_mode))
			{
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"type\tdirectory", 0);
				mode[0] = 'd';
			}
			else if (S_ISLNK(buf.stx_mode))
			{
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"type\tsymlink", 0);
				mode[0] = 'l';
			}
			else if (S_ISCHR(buf.stx_mode))
			{
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"type\tcharacter device", 0);
				mode[0] = 'c';
			}
			else if (S_ISBLK(buf.stx_mode))
			{
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"type\tblock device", 0);
				mode[0] = 'b';
			}
			else if (S_ISFIFO(buf.stx_mode))
			{
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"type\tFIFO/pipe", 0);
				mode[0] = 'f';
			}
			else if (S_ISSOCK(buf.stx_mode))
			{
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"type\tsocket", 0);
				mode[0] = 's';
			}
			else
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"type\tunknown", 0);

			int i = 0;
			const char *units[] = {"", "KB", "MB", "GB", "TB"};
			double size = (double)buf.stx_size;

			while (size > 1024)
			{
				size /= 1024;
				i++;
			}

			if (i > 0 && i < ARRAY_SIZE(units))
				snprintf(text, sizeof(text), "size\t%.1f %s (%'lld bytes)", size, units[i], buf.stx_size);
			else
				snprintf(text, sizeof(text), "size\t%'lld bytes", buf.stx_size);

			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);

			time_t t = (time_t)buf.stx_mtime.tv_sec;
			strftime(text, sizeof(text), "mtime\t%Y-%m-%d %H:%M:%S (%Z)", localtime(&t));
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);

			t = (time_t)buf.stx_atime.tv_sec;
			strftime(text, sizeof(text), "atime\t%Y-%m-%d %H:%M:%S (%Z)", localtime(&t));
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);

			t = (time_t)buf.stx_ctime.tv_sec;
			strftime(text, sizeof(text), "ctime\t%Y-%m-%d %H:%M:%S (%Z)", localtime(&t));

			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);

			t = (time_t)buf.stx_btime.tv_sec;
			strftime(text, sizeof(text), "btime\t%Y-%m-%d %H:%M:%S (%Z)", localtime(&t));
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);

			i = 1;

			if (buf.stx_mode & S_IRUSR)
				mode[i] = 'r';

			i++;

			if (buf.stx_mode & S_IWUSR)
				mode[i] = 'w';

			i++;

			if (buf.stx_mode & S_ISUID)
				mode[i] = 'S';
			else if (buf.stx_mode & S_IXUSR)
				mode[i] = 'x';

			i++;

			if (buf.stx_mode & S_IRGRP)
				mode[i] = 'r';

			i++;

			if (buf.stx_mode & S_IWGRP)
				mode[i] = 'w';

			i++;

			if (buf.stx_mode & S_ISGID)
				mode[i] = 'S';
			else if (buf.stx_mode & S_IXGRP)
				mode[i] = 'x';

			i++;

			if (buf.stx_mode & S_IROTH)
				mode[i] = 'r';

			i++;

			if (buf.stx_mode & S_IWOTH)
				mode[i] = 'w';

			i++;

			if (buf.stx_mode & S_ISVTX)
				mode[i] = 'T';
			else if (buf.stx_mode & S_IXOTH)
				mode[i] = 'x';

			snprintf(text, sizeof(text), "mode\t%o (%s)",  buf.stx_mode & (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX), mode);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);

			snprintf(text, sizeof(text), "access\t%c%c%c",
			         (access(gRemoteName, R_OK) == 0) ? 'r' : '-',
			         (access(gRemoteName, W_OK) == 0) ? 'w' : '-',
			         (access(gRemoteName, X_OK) == 0) ? 'x' : '-');
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);

			struct passwd *pw = getpwuid(buf.stx_uid);

			if (pw)
			{
				snprintf(text, sizeof(text), "owner\t%s (%d)",  pw->pw_name, buf.stx_uid);
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			}

			struct group  *gr = getgrgid(buf.stx_gid);

			if (gr)
			{
				snprintf(text, sizeof(text), "group\t%s (%d)",  gr->gr_name, buf.stx_gid);
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			}

			if (buf.stx_rdev_major != 0)
			{
				snprintf(text, sizeof(text), "attributes\t%lld",  buf.stx_attributes);
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			}

			snprintf(text, sizeof(text), "dev\t%d,%d",  buf.stx_dev_major, buf.stx_dev_minor);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);

			if (buf.stx_rdev_major != 0)
			{
				snprintf(text, sizeof(text), "rdev\t%d,%d",  buf.stx_rdev_major, buf.stx_rdev_minor);
				SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			}

			snprintf(text, sizeof(text), "inode\t%llu",  buf.stx_ino);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "blksize\t%'d",  buf.stx_blksize);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "blocks\t%'lld",  buf.stx_blocks);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
			snprintf(text, sizeof(text), "nlink\t%u",  buf.stx_nlink);
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
		}

		char list[XATTR_SIZE_MAX];
		ssize_t len = listxattr(gRemoteName, list, XATTR_SIZE_MAX);

		if (len > 0)
		{
			SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)"xattrs:", 0);

			for (int i = 0; i < len; i += strlen(&list[i]) + 1)
			{
				char value[XATTR_SIZE_MAX];
				ssize_t value_len = getxattr(gRemoteName, &list[i], value, XATTR_SIZE_MAX);

				if (value_len > -1)
				{
					snprintf(text, sizeof(text), "%.99s\t%.299s",  &list[i], value);
					SendDlgMsg(pDlg, "lbProps", DM_LISTADDSTR, (intptr_t)text, 0);
				}
			}
		}
	}

	return 0;
}

intptr_t DCPCALL OptionsDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	FILE *fp;
	size_t len = 0;
	ssize_t read = 0;
	int i = 0;
	char *line = NULL;

	switch (Msg)
	{
	case DN_INITDIALOG:
		if ((fp = fopen(gHistoryFile, "r")) != NULL)
		{
			while (((read = getline(&line, &len, fp)) != -1))
			{
				if (i == 1)
					SendDlgMsg(pDlg, "lbHistory", DM_SHOWITEM, 1, 0);
				else if (i > MAXINIITEMS)
					break;

				if (line[read - 1] == '\n')
					line[read - 1] = '\0';

				if (line != NULL && line[0] != '\0' && (strcmp(gStartPath, line) != 0))
				{
					if (i == 0)
						SendDlgMsg(pDlg, "fnSelectPath", DM_SETTEXT, (intptr_t)line, 0);

					SendDlgMsg(pDlg, "lbHistory", DM_LISTADD, (intptr_t)line, 0);
					i++;
				}
			}

			free(line);
			fclose(fp);
		}

		if (gStartPath[0] != '\0')
		{
			SendDlgMsg(pDlg, "fnSelectPath", DM_SETTEXT, (intptr_t)gStartPath, 0);
			SendDlgMsg(pDlg, "lbHistory", DM_LISTINSERT, 0, (intptr_t)gStartPath);

			if (SendDlgMsg(pDlg, "lbHistory", DM_LISTGETCOUNT, 0, 0) > 1)
				SendDlgMsg(pDlg, "lbHistory", DM_SHOWITEM, 1, 0);
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			char *path = (char*)SendDlgMsg(pDlg, "fnSelectPath", DM_GETTEXT, 0, 0);

			if (path && strlen(path) > 0)
			{
				if (strcmp(gStartPath, path) != 0)
					snprintf(gStartPath, sizeof(gStartPath), "%s", path);
			}

			if ((fp = fopen(gHistoryFile, "w")) != NULL)
			{
				fprintf(fp, "%s\n", gStartPath);
				size_t count = (size_t)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETCOUNT, 0, 0);

				for (i = 0; i < count; i++)
				{
					if (i > MAXINIITEMS)
						break;

					line = (char*)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEM, i, 0);

					if (line != NULL && line[0] != '\0' && (strcmp(gStartPath, line) != 0))
						fprintf(fp, "%s\n", line);
				}

				fclose(fp);
			}
		}
		else if (strcmp(DlgItemName, "lbHistory") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				char *path = (char*)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEM, i, 0);
				SendDlgMsg(pDlg, "fnSelectPath", DM_SETTEXT, (intptr_t)path, 0);
			}
		}

		break;

	case DN_KEYUP:
		if (strcmp(DlgItemName, "lbHistory") == 0)
		{
			int16_t *key = (int16_t*)wParam;

			if (lParam == 1 && *key == 46)
			{
				int i = (int)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEMINDEX, 0, 0);

				if (i != -1)
					SendDlgMsg(pDlg, "lbHistory", DM_LISTDELETE, i, 0);
			}
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
	                       "object OptDialogBox: TOptDialogBox\n"
	                       "  Left = 458\n"
	                       "  Height = 262\n"
	                       "  Top = 307\n"
	                       "  Width = 672\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Options'\n"
	                       "  ChildSizing.LeftRightSpacing = 15\n"
	                       "  ChildSizing.TopBottomSpacing = 15\n"
	                       "  ChildSizing.VerticalSpacing = 10\n"
	                       "  ClientHeight = 262\n"
	                       "  ClientWidth = 672\n"
	                       "  DesignTimePPI = 100\n"
	                       "  OnShow = DialogBoxShow\n"
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object lblStartPath: TLabel\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    Left = 15\n"
	                       "    Height = 49\n"
	                       "    Top = 15\n"
	                       "    Width = 638\n"
	                       "    BorderSpacing.Right = 5\n"
	                       "    Caption = 'Please select the folder you want to work with.'#10#10'You can re-open this dialog by clicking plugin properties or \"..\" properties in the root folder.'\n"
	                       "    ParentColor = False\n"
	                       "  end\n"
	                       "  object lbHistory: TListBox\n"
	                       "    AnchorSideLeft.Control = fnSelectPath\n"
	                       "    AnchorSideTop.Control = lblStartPath\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = lblStartPath\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 83\n"
	                       "    Top = 74\n"
	                       "    Width = 638\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    ItemHeight = 0\n"
	                       "    OnClick = ListBoxClick\n"
	                       "    OnKeyUp = ListBoxKeyUp\n"
	                       "    TabOrder = 0\n"
	                       "    Visible = False\n"
	                       "  end\n"
	                       "  object fnSelectPath: TDirectoryEdit\n"
	                       "    AnchorSideLeft.Control = lblStartPath\n"
	                       "    AnchorSideTop.Control = lbHistory\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = lblStartPath\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 36\n"
	                       "    Top = 167\n"
	                       "    Width = 638\n"
	                       "    Directory = '/'\n"
	                       "    DialogTitle = 'Select folder'\n"
	                       "    ShowHidden = False\n"
	                       "    ButtonWidth = 24\n"
	                       "    NumGlyphs = 1\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    BorderSpacing.Bottom = 1\n"
	                       "    MaxLength = 0\n"
	                       "    TabOrder = 1\n"
	                       "    Text = '/'\n"
	                       "  end\n"
	                       "  object btnCancel: TBitBtn\n"
	                       "    AnchorSideTop.Control = btnOK\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    Left = 446\n"
	                       "    Height = 31\n"
	                       "    Top = 218\n"
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
	                       "    TabOrder = 3\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = fnSelectPath\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = fnSelectPath\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 552\n"
	                       "    Height = 31\n"
	                       "    Top = 218\n"
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
	                       "    TabOrder = 2\n"
	                       "  end\n"
	                       "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), OptionsDlgProc);
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

		snprintf(FindData->cFileName, MAX_PATH - 1, "%s", ent->d_name);

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

	if (strcmp(gStartPath, "/") == 0)
		gStartPath[0] = '\0';

	dirdata = malloc(sizeof(tVFSDirData));

	if (dirdata == NULL)
		return (HANDLE)(-1);

	memset(dirdata, 0, sizeof(tVFSDirData));
	snprintf(dirdata->path, sizeof(dirdata->path), "%s%s", gStartPath, Path);

	if ((dir = opendir(dirdata->path)) == NULL)
	{
		int errsv = errno;

		if (gFileListOP)
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
	char localpath[PATH_MAX];
	snprintf(localpath, PATH_MAX, "%s%s", gStartPath, RemoteName);
	snprintf(RemoteName, maxlen - 1, "%s", localpath);
	return TRUE;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	char realname[PATH_MAX];
	BuildCasePath(RemoteName, realname);

	if (strcmp(LocalName, realname) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (CopyFlags == 0 && CaseDuplExists(realname, FALSE) == FS_FILE_EXISTS)
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

	if (!OverWrite)
	{
		if (!Move)
		{
			if (CaseDuplExists(newpath, FALSE) == FS_FILE_EXISTS)
				return FS_FILE_EXISTS;
			else
			{
				gCaseName[0] = '\0';

				if (access(newpath, F_OK) == 0)
					return FS_FILE_EXISTS;
			}
		}
	}

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

	if (CaseDuplExists(realname, FALSE) != FS_FILE_EXISTS && mkdir(realname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
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
			OptionsDialog();
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

void DCPCALL FsStatusInfo(char* RemoteDir, int InfoStartEnd, int InfoOperation)
{
	if (InfoOperation == FS_STATUS_OP_LIST)
		gFileListOP = (InfoStartEnd == FS_STATUS_START);
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex > 3)
		return ft_nomorefields;

	Units[0] = '\0';

	if (FieldIndex == 0)
		snprintf(FieldName, maxlen - 1, "casedupl");
	else if (FieldIndex == 1)
		snprintf(FieldName, maxlen - 1, "DOSATTRIB");
	else if (FieldIndex == 2)
		snprintf(FieldName, maxlen - 1, "object");
	else if (FieldIndex == 3)
		snprintf(FieldName, maxlen - 1, "access");

	return ft_string;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	char realname[PATH_MAX];
	snprintf(realname, sizeof(realname), "%s%s", gStartPath, FileName);

	if (FieldIndex == 0 && CaseDuplExists(realname, TRUE) == FS_FILE_EXISTS)
	{
		sprintf((char*)FieldValue, "❗");
		return ft_string;
	}
	else if (FieldIndex == 1)
	{
		char value[XATTR_SIZE_MAX];
		ssize_t value_len = getxattr(realname, "user.DOSATTRIB", value, XATTR_SIZE_MAX);

		if (value_len > 0)
		{
			value[value_len] = '\0';
			int num = (int)strtol(value, NULL, 16);
			snprintf((char*)FieldValue, maxlen, "%s%s%s%s", num & 0x1 ? "Ⓡ" : "㊀", num & 0x20 ? "Ⓐ" : "㊀", num & 0x2 ? "Ⓗ" : "㊀", num & 0x4 ? "Ⓢ" : "㊀");
			return ft_string;
		}
	}
	else if (FieldIndex == 2)
	{
		struct stat buf;

		if (lstat(realname, &buf) == 0)
		{
			if (S_ISREG(buf.st_mode))
				sprintf((char*)FieldValue, "🅁");
			else if (S_ISDIR(buf.st_mode))
				sprintf((char*)FieldValue, "🄳");
			else if (S_ISLNK(buf.st_mode))
				sprintf((char*)FieldValue, "🄻");
			else if (S_ISCHR(buf.st_mode))
				sprintf((char*)FieldValue, "🄲");
			else if (S_ISBLK(buf.st_mode))
				sprintf((char*)FieldValue, "🄱");
			else if (S_ISFIFO(buf.st_mode))
				sprintf((char*)FieldValue, "🄵");
			else if (S_ISSOCK(buf.st_mode))
				sprintf((char*)FieldValue, "🅂");

			return ft_string;
		}
	}
	else if (FieldIndex == 3)
	{
		snprintf((char*)FieldValue, maxlen - 1, "%s%s%s",
		         (access(realname, R_OK) == 0) ? "Ⓡ" : "㊀",
		         (access(realname, W_OK) == 0) ? "Ⓦ" : "㊀",
		         (access(realname, X_OK) == 0) ? "Ⓧ" : "㊀");
		return ft_string;
	}

	return ft_fieldempty;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	snprintf(ViewContents, maxlen - 1,
	         "[DC().GETFILESIZE{}]\\n[DC().GETFILETIME{}]\\n[DC().GETFILEATTR{OCTAL}] [Plugin(FS).access{}] [Plugin(FS).object{}] [Plugin(FS).DOSATTRIB{}]\\n[Plugin(FS).casedupl{}]");
	snprintf(ViewHeaders,  maxlen - 1, "Size\\nDate\\nAttr\\n!!!");
	snprintf(ViewWidths,   maxlen - 1, "100,15,-25,40,41,-6");
	snprintf(ViewOptions,  maxlen - 1, "-1|0");
	return TRUE;
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	snprintf(gHistoryFile, PATH_MAX, "%s", dps->DefaultIniName);
	char *pos = strrchr(gHistoryFile, '/');

	if (pos)
		strcpy(pos + 1, HISTNAME);
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	snprintf(DefRootName, maxlen - 1, ROOTNAME);
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

}
