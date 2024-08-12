#define _GNU_SOURCE
#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <time.h>
#include <utime.h>
#include <ftw.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

#define BUFSIZE 8192
#define RE_PATTERN "^\\-\\s(?'date'\\d{4}\\-\\d{2}\\-\\d{2}\\s\\d{2}:\\d{2}:\\d{2})\\s+\
(?'size'\\d+)\\s(?'attr'\\s?d?[DRASHI\\d]*)\\s+(?'name'[^\\n]+)"
#define ZPAQ_ERRPASS "zpaq error: password incorrect"
#define MSG_PASS "Enter password:"

#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox

typedef struct sArcData
{
	GRegex *re;
	gchar *arc;
	gchar *tmpdir;
	gchar *output;
	gboolean extract;
	GMatchInfo *match_info;
	tChangeVolProc ChangeVolProc;
	tProcessDataProc ProcessDataProc;
} tArcData;

typedef tArcData* ArcData;
typedef void *HINSTANCE;

tChangeVolProc gChangeVolProc  = NULL;
tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;
static char gOptVerNum[3] = "0";
static char gOptMethod[3] = "1";
static char gOptThreads[3] = "0";
static char gPass[MAX_PATH] = "";
static char gPassLastFile[PATH_MAX] = "";


intptr_t DCPCALL OptDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	int index;
	char *value;

	switch (Msg)
	{
	case DN_INITDIALOG:
		if (strcmp(gOptVerNum, "0") != 0)
		{
			SendDlgMsg(pDlg, "edDigits", DM_SETTEXT, (intptr_t)gOptVerNum, 0);
			SendDlgMsg(pDlg, "chAll", DM_SETCHECK, 1, 0);
		}

		SendDlgMsg(pDlg, "edThreads", DM_SETTEXT, (intptr_t)gOptThreads, 0);
		index = atoi(gOptMethod);
		SendDlgMsg(pDlg, "cbCompr", DM_LISTSETITEMINDEX, (intptr_t)index, 0);
		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			if (SendDlgMsg(pDlg, "chAll", DM_GETCHECK, 0, 0) == 0)
				g_strlcpy(gOptVerNum, "0", 3);
			else
			{
				value = (char*)SendDlgMsg(pDlg, "edDigits", DM_GETTEXT, 0, 0);
				g_strlcpy(gOptVerNum, value, 3);
			}

			value = (char*)SendDlgMsg(pDlg, "edThreads", DM_GETTEXT, 0, 0);
			g_strlcpy(gOptThreads, value, 3);
			index = SendDlgMsg(pDlg, "cbCompr", DM_LISTGETITEMINDEX, 0, 0);
			snprintf(gOptMethod, 3, "%d", index);
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "chAll") == 0)
		{
			SendDlgMsg(pDlg, "lblDigits", DM_ENABLE, wParam, 0);
			SendDlgMsg(pDlg, "edDigits", DM_ENABLE, wParam, 0);
		}

		break;
	}

	return 0;
}

static void remove_file(const char *file)
{
	if (remove(file) == -1)
	{
		int errsv = errno;
		printf("ERR(%s) remove '%s': %s\n", PLUGNAME, file, strerror(errsv));
	}
}

static int nftw_remove_cb(const char *file, const struct stat *bif, int tflag, struct FTW *ftwbuf)
{
	remove_file(file);
	return 0;
}

static void remove_target(const char *filename)
{
	struct stat st;

	if (lstat(filename, &st) == 0)
	{
		if S_ISDIR(st.st_mode)
			nftw(filename, nftw_remove_cb, 13, FTW_DEPTH | FTW_PHYS);
		else
			remove_file(filename);
	}
}

static int copy_file(char* infile, char* outfile)
{
	int ifd, ofd, done;
	ssize_t len, total = 0;
	char buff[BUFSIZE];
	struct stat buf;
	int result = E_SUCCESS;

	if (strcmp(infile, outfile) == 0)
		return E_NOT_SUPPORTED;

	if (stat(infile, &buf) != 0)
		return E_NOT_SUPPORTED;

	ifd = open(infile, O_RDONLY);

	if (ifd == -1)
		return E_EREAD;

	ofd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (ofd > -1)
	{
		while ((len = read(ifd, buff, sizeof(buff))) > 0)
		{
			if (write(ofd, buff, len) == -1)
			{
				result = E_EWRITE;
				break;
			}

			total += len;

			if (buf.st_size > 0)
				done = total * 100 / buf.st_size;
			else
				done = 0;

			if (done > 100)
				done = 100;

			if (gProcessDataProc(infile, -(1000 + done)) == 0)
			{
				result = E_EABORTED;
				break;
			}
		}

		close(ofd);
		chmod(outfile, buf.st_mode);
	}
	else
		result = E_EWRITE;

	close(ifd);

	return result;
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	gchar **argv;
	gint status = 0;
	gchar *stdout = NULL;
	gchar *stderr = NULL;
	GError *err = NULL;

	gchar *argv_nopass[] = { "zpaq", "l", ArchiveData->ArcName, "-all", gOptVerNum, NULL };
	gchar *argv_pass[] = { "zpaq", "l", ArchiveData->ArcName, "-all", gOptVerNum, "-key", gPass, NULL };

	if (strcmp(gPassLastFile, ArchiveData->ArcName) != 0 && ArchiveData->OpenMode == PK_OM_LIST)
		gPass[0] = '\0';

	if (strcmp(gPassLastFile, ArchiveData->ArcName) == 0 && gPass[0] != '\0')
		argv = argv_pass;
	else
		argv = argv_nopass;

	if (!g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout, &stderr, &status, &err))
	{
		if (err)
		{
			MessageBox(err->message, PLUGNAME,  MB_OK | MB_ICONERROR);
			g_error_free(err);
		}

		g_free(stdout);
		g_free(stderr);
		ArchiveData->OpenResult = E_EOPEN;
		return E_SUCCESS;
	}

	if (status != 0 && stderr)
	{
		g_free(stdout);
		stdout = NULL;

		if (strncmp(stderr, ZPAQ_ERRPASS, strlen(ZPAQ_ERRPASS)) == 0 && InputBox(PLUGNAME, MSG_PASS, TRUE, gPass, MAX_PATH))
		{
			g_strlcpy(gPassLastFile, ArchiveData->ArcName, PATH_MAX);
			argv = argv_pass;

			if (!g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout, NULL, &status, NULL) || status != 0)
			{
				gPass[0] = '\0';
				g_free(stdout);
				g_free(stderr);
				ArchiveData->OpenResult = E_EOPEN;
				return E_SUCCESS;
			}
		}
		else
		{
			gPass[0] = '\0';
			g_free(stderr);
			ArchiveData->OpenResult = E_EOPEN;
			return E_SUCCESS;
		}
	}

	g_free(stderr);

	tArcData *data = (tArcData*)malloc(sizeof(tArcData));

	if (data == NULL)
	{
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	memset(data, 0, sizeof(tArcData));

	if (stdout)
	{
		g_clear_error(&err);
		data->arc = g_strdup(ArchiveData->ArcName);
		data->output = stdout;
		data->re = g_regex_new(RE_PATTERN, G_REGEX_MULTILINE, 0, &err);

		if (data->re)
			g_regex_match(data->re, data->output, 0, &data->match_info);
		else
			MessageBox(err->message, PLUGNAME,  MB_OK | MB_ICONERROR);

		if (err)
			g_error_free(err);
	}
	else
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;

	return data;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	ArcData data = (ArcData)hArcData;

	gchar *string;

	if (g_match_info_matches(data->match_info))
	{
		string = g_match_info_fetch_named(data->match_info, "name");

		if (string)
			g_strlcpy(HeaderDataEx->FileName, string, sizeof(HeaderDataEx->FileName) - 1);
		else
			g_strlcpy(HeaderDataEx->FileName, "<ERROR>", sizeof(HeaderDataEx->FileName) - 1);

		g_free(string);

		string = g_match_info_fetch_named(data->match_info, "attr");

		if (string)
		{
			if (string[0] == 'd')
			{
				HeaderDataEx->FileAttr = S_IFDIR;
				HeaderDataEx->FileAttr |= strtol(string + 1, NULL, 8);
			}
			else
				HeaderDataEx->FileAttr = strtol(string, NULL, 8);
		}

		g_free(string);

		string = g_match_info_fetch_named(data->match_info, "size");

		if (string)
		{
			gdouble filesize = g_ascii_strtod(string, NULL);
			HeaderDataEx->UnpSizeHigh = ((int64_t)filesize & 0xFFFFFFFF00000000) >> 32;
			HeaderDataEx->UnpSize = (int64_t)filesize & 0x00000000FFFFFFFF;
		}

		g_free(string);

		HeaderDataEx->PackSizeHigh = 0xFFFFFFFF;
		HeaderDataEx->PackSize = 0xFFFFFFFE;

		string = g_match_info_fetch_named(data->match_info, "date");

		if (string)
		{
			struct tm tm = {0};

			if (strptime(string, "%Y-%m-%d %R", &tm))
			{
				time_t utc = mktime(&tm);
				HeaderDataEx->FileTime = (int)(utc - timezone + (daylight ? 3600 : 0));
			}
		}

		g_free(string);

		return E_SUCCESS;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	int result = E_SUCCESS;
	ArcData data = (ArcData)hArcData;

	if (data->ProcessDataProc(DestName, -1000) == 0)
		result = E_EABORTED;
	else if (Operation == PK_EXTRACT)
	{
		gchar *arc_path = g_match_info_fetch_named(data->match_info, "name");

		if (!data->extract)
		{
			gchar *outdir = g_path_get_dirname(DestName);
			data->tmpdir = tempnam(outdir, "arc_");
			mkdir(data->tmpdir, 0755);
			g_free(outdir);
			data->extract = TRUE;
		}

		GPid pid;
		gint status = 0;
		gchar *args = g_strdup_printf("zpaq\nx\n%s\n%s\n-all\n%s\n-threads\n%s", data->arc, arc_path, gOptVerNum, gOptThreads);

		if (gPass[0] != '\0')
		{
			gchar *tmp = g_strdup_printf("%s\n-key\n%s", args, gPass);
			g_free(args);
			args = tmp;
		}

		gchar **argv = g_strsplit(args, "\n", -1);
		g_free(args);

		if (!g_spawn_async(data->tmpdir, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &pid, NULL))
			result = E_EWRITE;
		else
		{
			while (data->ProcessDataProc(DestName, -1050) != 0 && kill(pid, 0) == 0)
				sleep(0.5);

			if (data->ProcessDataProc(DestName, -1050) == 0 && kill(pid, 0) == 0)
			{
				kill(pid, SIGTERM);
				result = E_EABORTED;
			}

			waitpid(pid, &status, 0);
			g_spawn_close_pid(pid);

			if (result == E_SUCCESS && status != 0)
				result = E_EWRITE;
		}

		g_strfreev(argv);

		if (result == E_SUCCESS)
		{
			gchar *file = g_strdup_printf("%s/%s", data->tmpdir, arc_path);

			if (rename(file, DestName) != 0)
				result = E_EWRITE;

			g_free(file);
		}

		g_free(arc_path);
	}
	else if (Operation == PK_TEST)
		result = E_NOT_SUPPORTED;

	g_match_info_next(data->match_info, NULL);

	return result;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData data = (ArcData)hArcData;

	g_match_info_free(data->match_info);
	g_regex_unref(data->re);
	remove_target(data->tmpdir);
	g_free(data->output);
	g_free(data->tmpdir);
	g_free(data->arc);
	g_free(data);

	return E_SUCCESS;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	GPid pid;
	DIR *dir;
	struct stat buf;
	struct dirent *ent;
	int result = E_SUCCESS;
	gchar *filelist = AddList;
	gchar *arcdir = NULL;
	GError *err = NULL;
	gint status = 0;

	if (Flags & PK_PACK_MOVE_FILES || !(Flags & PK_PACK_SAVE_PATHS))
		return E_NOT_SUPPORTED;

	if (strcmp(gPassLastFile, PackedFile) != 0)
		gPass[0] = '\0';

	if (Flags & PK_PACK_ENCRYPT)
	{
		if (InputBox(PLUGNAME, MSG_PASS, TRUE, gPass, MAX_PATH))
			g_strlcpy(gPassLastFile, PackedFile, PATH_MAX);
		else
			gPass[0] = '\0';
	}

	gchar *outdir = g_path_get_dirname(PackedFile);
	gchar *tmpdir = tempnam(outdir, "arc_");
	g_free(outdir);

	if (mkdir(tmpdir, 0755) != 0)
	{
		g_free(tmpdir);
		return E_ECREATE;
	}

	if (SubPath && strcmp(gOptVerNum, "0") != 0)
	{
		gchar **split = g_strsplit(SubPath, "/", -1);

		if (split)
		{
			arcdir = g_strjoinv("/", split + 1);
			g_strfreev(split);
		}
	}
	else if (SubPath)
		arcdir = g_strdup(SubPath);

	while (*filelist)
	{
		gchar *infile = g_strdup_printf("%s%s", SrcPath, filelist);
		gchar *tmpfile = g_strdup_printf("%s%s%s/%s", tmpdir, arcdir ? "/" : "", arcdir ? arcdir : "", filelist);

		if (filelist[strlen(filelist) - 1] != '/')
			result = copy_file(infile, tmpfile);
		else if (mkdir(tmpfile, 0755) != 0)
			result = E_ECREATE;

		g_free(infile);
		g_free(tmpfile);

		if (result != E_SUCCESS)
			break;

		while (*filelist++);
	}

	filelist = AddList;

	while (*filelist)
	{
		gchar *infile = g_strdup_printf("%s%s", SrcPath, filelist);
		gchar *tmpfile = g_strdup_printf("%s%s%s/%s", tmpdir, arcdir ? "/" : "", arcdir ? arcdir : "", filelist);

		if (stat(infile, &buf) == 0)
		{
			struct utimbuf ubuf;
			ubuf.actime = buf.st_atime;
			ubuf.modtime = buf.st_mtime;
			utime(tmpfile, &ubuf);
		}

		g_free(infile);
		g_free(tmpfile);

		while (*filelist++);
	}

	if (result == E_SUCCESS)
	{
		gchar *args = g_strdup_printf("zpaq\na\n%s", PackedFile);

		if ((dir = opendir(tmpdir)) != NULL)
		{
			while ((ent = readdir(dir)) != NULL)
			{
				if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
				{
					gchar *tmp = g_strdup_printf("%s\n%s", args, ent->d_name);
					g_free(args);
					args = tmp;
				}
			}
		}

		gchar *tmp = g_strdup_printf("%s\n-m%s\n-t%s", args, gOptMethod, gOptThreads);
		g_free(args);

		if (gPass[0] != '\0')
		{
			args = g_strdup_printf("%s\n-key\n%s", tmp, gPass);
			g_free(tmp);
		}
		else
			args = tmp;


		gchar **argv = g_strsplit(args, "\n", -1);
		g_free(args);

		if (!g_spawn_async(tmpdir, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &pid, &err))
		{
			if (err)
			{
				MessageBox(err->message, PLUGNAME,  MB_OK | MB_ICONERROR);
				g_error_free(err);
			}

			result = E_EABORTED;
		}
		else
		{
			while (gProcessDataProc(tmpdir, -50) != 0 && kill(pid, 0) == 0)
				sleep(0.5);

			if (gProcessDataProc(tmpdir, -50) == 0 && kill(pid, 0) == 0)
			{
				kill(pid, SIGTERM);
				result = E_EABORTED;
			}

			waitpid(pid, &status, 0);
			g_spawn_close_pid(pid);

			if (result == E_SUCCESS && status != 0)
				result = E_EWRITE;
		}

		g_strfreev(argv);
	}

	gProcessDataProc(tmpdir, -100);
	remove_target(tmpdir);
	g_free(tmpdir);
	g_free(arcdir);

	return result;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData data = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		gProcessDataProc = pProcessDataProc;
	else
		data->ProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc)
{
	ArcData data = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		gChangeVolProc = pChangeVolProc;
	else
		data->ChangeVolProc = pChangeVolProc;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_NEW | PK_CAPS_MODIFY | PK_CAPS_MULTIPLE | PK_CAPS_SEARCHTEXT | PK_CAPS_OPTIONS | PK_CAPS_ENCRYPT;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	static char lfm_data[] = "object DialogBox: TDialogBox\n"
	                         "  Left = 404\n"
	                         "  Height = 391\n"
	                         "  Top = 142\n"
	                         "  Width = 447\n"
	                         "  AutoSize = True\n"
	                         "  BorderStyle = bsDialog\n"
	                         "  Caption = 'Options'\n"
	                         "  ChildSizing.LeftRightSpacing = 20\n"
	                         "  ChildSizing.TopBottomSpacing = 20\n"
	                         "  ClientHeight = 391\n"
	                         "  ClientWidth = 447\n"
	                         "  OnClose = DialogBoxClose\n"
	                         "  OnShow = DialogBoxShow\n"
	                         "  Position = poOwnerFormCenter\n"
	                         "  LCLVersion = '3.0.0.3'\n"
	                         "  object chAll: TCheckBox\n"
	                         "    AnchorSideLeft.Control = Owner\n"
	                         "    AnchorSideTop.Control = edDigits\n"
	                         "    AnchorSideTop.Side = asrCenter\n"
	                         "    Left = 20\n"
	                         "    Height = 23\n"
	                         "    Top = 23\n"
	                         "    Width = 150\n"
	                         "    Caption = 'Show all file versions'\n"
	                         "    TabOrder = 0\n"
	                         "    OnChange = CheckBoxChange\n"
	                         "  end\n"
	                         "  object lblDigits: TLabel\n"
	                         "    AnchorSideLeft.Control = chAll\n"
	                         "    AnchorSideLeft.Side = asrBottom\n"
	                         "    AnchorSideTop.Control = edDigits\n"
	                         "    AnchorSideTop.Side = asrCenter\n"
	                         "    Left = 190\n"
	                         "    Height = 16\n"
	                         "    Top = 26\n"
	                         "    Width = 96\n"
	                         "    BorderSpacing.Left = 20\n"
	                         "    BorderSpacing.Right = 5\n"
	                         "    Caption = 'Number of digits'\n"
	                         "    Enabled = False\n"
	                         "  end\n"
	                         "  object edDigits: TEdit\n"
	                         "    AnchorSideLeft.Control = lblDigits\n"
	                         "    AnchorSideLeft.Side = asrBottom\n"
	                         "    AnchorSideTop.Control = Owner\n"
	                         "    AnchorSideRight.Side = asrBottom\n"
	                         "    Left = 291\n"
	                         "    Height = 28\n"
	                         "    Top = 20\n"
	                         "    Width = 49\n"
	                         "    Alignment = taRightJustify\n"
	                         "    BorderSpacing.Left = 5\n"
	                         "    Enabled = False\n"
	                         "    MaxLength = 3\n"
	                         "    TabOrder = 1\n"
	                         "    Text = '4'\n"
	                         "  end\n"
	                         "  object gbCompr: TGroupBox\n"
	                         "    AnchorSideLeft.Control = Owner\n"
	                         "    AnchorSideTop.Control = edDigits\n"
	                         "    AnchorSideTop.Side = asrBottom\n"
	                         "    AnchorSideRight.Control = edDigits\n"
	                         "    AnchorSideRight.Side = asrBottom\n"
	                         "    Left = 20\n"
	                         "    Height = 96\n"
	                         "    Top = 68\n"
	                         "    Width = 320\n"
	                         "    Anchors = [akTop, akLeft, akRight]\n"
	                         "    AutoSize = True\n"
	                         "    BorderSpacing.Top = 20\n"
	                         "    Caption = 'Compression'\n"
	                         "    ChildSizing.LeftRightSpacing = 10\n"
	                         "    ChildSizing.TopBottomSpacing = 10\n"
	                         "    ChildSizing.HorizontalSpacing = 5\n"
	                         "    ChildSizing.VerticalSpacing = 5\n"
	                         "    ChildSizing.EnlargeHorizontal = crsHomogenousChildResize\n"
	                         "    ChildSizing.Layout = cclLeftToRightThenTopToBottom\n"
	                         "    ChildSizing.ControlsPerLine = 2\n"
	                         "    ClientHeight = 79\n"
	                         "    ClientWidth = 318\n"
	                         "    TabOrder = 2\n"
	                         "    object lblCompr: TLabel\n"
	                         "      AnchorSideTop.Side = asrCenter\n"
	                         "      Left = 10\n"
	                         "      Height = 26\n"
	                         "      Top = 10\n"
	                         "      Width = 142\n"
	                         "      Alignment = taCenter\n"
	                         "      Caption = 'Compress level'\n"
	                         "      Layout = tlCenter\n"
	                         "    end\n"
	                         "    object cbCompr: TComboBox\n"
	                         "      Left = 157\n"
	                         "      Height = 26\n"
	                         "      Top = 10\n"
	                         "      Width = 151\n"
	                         "      ItemHeight = 0\n"
	                         "      Items.Strings = (\n"
	                         "        '0 (fast)'\n"
	                         "        '1 (default)'\n"
	                         "        '2'\n"
	                         "        '3'\n"
	                         "        '4'\n"
	                         "        '5 (best)'\n"
	                         "      )\n"
	                         "      Style = csDropDownList\n"
	                         "      TabOrder = 0\n"
	                         "    end\n"
	                         "    object lblThreads: TLabel\n"
	                         "      Left = 10\n"
	                         "      Height = 28\n"
	                         "      Top = 41\n"
	                         "      Width = 142\n"
	                         "      Alignment = taCenter\n"
	                         "      Caption = 'Use threads'\n"
	                         "      Layout = tlCenter\n"
	                         "    end\n"
	                         "    object edThreads: TEdit\n"
	                         "      Left = 157\n"
	                         "      Height = 28\n"
	                         "      Top = 41\n"
	                         "      Width = 151\n"
	                         "      Alignment = taRightJustify\n"
	                         "      MaxLength = 2\n"
	                         "      TabOrder = 1\n"
	                         "      Text = '0'\n"
	                         "    end\n"
	                         "  end\n"
	                         "  object btnCancel: TBitBtn\n"
	                         "    AnchorSideTop.Control = btnOK\n"
	                         "    AnchorSideTop.Side = asrCenter\n"
	                         "    AnchorSideRight.Control = btnOK\n"
	                         "    Left = 142\n"
	                         "    Height = 30\n"
	                         "    Top = 194\n"
	                         "    Width = 97\n"
	                         "    Anchors = [akTop, akRight]\n"
	                         "    BorderSpacing.Right = 4\n"
	                         "    Cancel = True\n"
	                         "    Constraints.MinHeight = 30\n"
	                         "    Constraints.MinWidth = 97\n"
	                         "    DefaultCaption = True\n"
	                         "    Kind = bkCancel\n"
	                         "    ModalResult = 2\n"
	                         "    OnClick = ButtonClick\n"
	                         "    TabOrder = 4\n"
	                         "  end\n"
	                         "  object btnOK: TBitBtn\n"
	                         "    AnchorSideTop.Control = gbCompr\n"
	                         "    AnchorSideTop.Side = asrBottom\n"
	                         "    AnchorSideRight.Control = gbCompr\n"
	                         "    AnchorSideRight.Side = asrBottom\n"
	                         "    Left = 243\n"
	                         "    Height = 30\n"
	                         "    Top = 194\n"
	                         "    Width = 97\n"
	                         "    Anchors = [akTop, akRight]\n"
	                         "    BorderSpacing.Top = 30\n"
	                         "    Constraints.MinHeight = 30\n"
	                         "    Constraints.MinWidth = 97\n"
	                         "    Default = True\n"
	                         "    DefaultCaption = True\n"
	                         "    Kind = bkOK\n"
	                         "    ModalResult = 1\n"
	                         "    OnClick = ButtonClick\n"
	                         "    TabOrder = 3\n"
	                         "  end\n"
	                         "end\n";

	gExtensions->DialogBoxLFM((intptr_t)lfm_data, (unsigned long)strlen(lfm_data), OptDlgProc);
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

