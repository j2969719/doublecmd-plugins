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
#define RE_LIST "^[\\-\\^+=#]\\s(?'date'\\d{4}\\-\\d{2}\\-\\d{2}\\s\\d{2}:\\d{2}:\\d{2})\
\\s+(?'size'\\-?\\d+)\\s(?'attr'\\s?d?[DRASHI\\d]*)\\s+(?'name'[^\\n]+)"
#define RE_INFO "\\d+/\\s\\+\\d+\\s\\-\\d\\s\\->\\s\\d+"
#define ZPAQ_ERRPASS "zpaq error: password incorrect"
#define MSG_PASS "Enter password:"
#define ERRFILE "<ERROR>"
#define HEADER "7kSt"
#define HEADER_SIZE 4
#define PK_CAPS PK_CAPS_NEW | PK_CAPS_MODIFY | PK_CAPS_MULTIPLE | PK_CAPS_SEARCHTEXT |\
 PK_CAPS_DELETE | PK_CAPS_OPTIONS | PK_CAPS_ENCRYPT | PK_CAPS_BY_CONTENT;

#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox

typedef struct sArcData
{
	GRegex *re;
	gchar *arc;
	gchar *output;
	GRegex *re_info;
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
gboolean gOptSkipInfo = TRUE;


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

		SendDlgMsg(pDlg, "chSkipInfo", DM_SETCHECK, (intptr_t)gOptSkipInfo, 0);
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
			gOptSkipInfo = (gboolean)SendDlgMsg(pDlg, "chSkipInfo", DM_GETCHECK, 0, 0);
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "chAll") == 0)
		{
			SendDlgMsg(pDlg, "lblDigits", DM_ENABLE, wParam, 0);
			SendDlgMsg(pDlg, "edDigits", DM_ENABLE, wParam, 0);
			SendDlgMsg(pDlg, "chSkipInfo", DM_ENABLE, wParam, 0);
		}

		break;
	}

	return 0;
}

static int parse_utcdate(char *string)
{
	int result = 0;
	struct tm tm = {0};

	if (strptime(string, "%Y-%m-%d %R", &tm))
	{
		time_t utc = mktime(&tm);
		result = (int)(utc - timezone + (daylight ? 3600 : 0));
	}

	return result;
}

int execute_archiver(char *workdir, char **argv, char *filename, tProcessDataProc process)
{
	GPid pid;
	gint status = 0;
	GError *err = NULL;
	int result = E_SUCCESS;

	if (!g_spawn_async(workdir, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &pid, &err))
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
		while (process(filename, -50) != 0 && kill(pid, 0) == 0)
			sleep(0.5);

		if (process(filename, -50) == 0 && kill(pid, 0) == 0)
		{
			kill(pid, SIGTERM);
			result = E_EABORTED;
		}

		waitpid(pid, &status, 0);
		g_spawn_close_pid(pid);

		if (result == E_SUCCESS && status != 0)
			result = E_EWRITE;
	}

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
		if (err)
			g_error_free(err);

		g_free(stdout);
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	memset(data, 0, sizeof(tArcData));

	if (stdout)
	{
		g_clear_error(&err);
		data->arc = g_strdup(ArchiveData->ArcName);
		data->output = stdout;
		data->re = g_regex_new(RE_LIST, G_REGEX_MULTILINE, 0, &err);

		if (data->re)
			g_regex_match(data->re, data->output, 0, &data->match_info);
		else
			MessageBox(err->message, PLUGNAME,  MB_OK | MB_ICONERROR);

		if (strcmp(gOptVerNum, "0") != 0)
			data->re_info = g_regex_new(RE_INFO, 0, 0, NULL);
	}
	else
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;

	if (err)
		g_error_free(err);

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

	while (g_match_info_matches(data->match_info))
	{
		string = g_match_info_fetch_named(data->match_info, "name");

		if (string)
		{
			if (data->re_info && gOptSkipInfo && g_regex_match(data->re_info, string, 0, NULL))
			{
				//g_free(string);
				//g_match_info_next(data->match_info, NULL);
				//continue;

				char *p = strrchr(string, '/');

				if (p)
					*p = '\0';

				g_strlcpy(HeaderDataEx->FileName, string, sizeof(HeaderDataEx->FileName) - 1);

				g_strlcpy(HeaderDataEx->CmtBuf, p + 1, HeaderDataEx->CmtBufSize);
				HeaderDataEx->CmtSize = (int)strlen(p + 1);

				g_free(string);

				string = g_match_info_fetch_named(data->match_info, "date");

				if (string)
					HeaderDataEx->FileTime = parse_utcdate(string);

				g_free(string);

				HeaderDataEx->FileAttr = S_IFDIR | 0755;
				return E_SUCCESS;
			}

			if (string[0] != '/')
				g_strlcpy(HeaderDataEx->FileName, string, sizeof(HeaderDataEx->FileName) - 1);
			else
				g_strlcpy(HeaderDataEx->FileName, ERRFILE, sizeof(HeaderDataEx->FileName) - 1);
		}
		else
			g_strlcpy(HeaderDataEx->FileName, ERRFILE, sizeof(HeaderDataEx->FileName) - 1);

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
			HeaderDataEx->FileTime = parse_utcdate(string);

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


		if (!arc_path) // || arc_path[0] == '/')
		{
			g_free(arc_path);
			g_match_info_next(data->match_info, NULL);
			return E_NOT_SUPPORTED;
		}

		if (data->re_info && !gOptSkipInfo && g_regex_match(data->re_info, arc_path, 0, NULL))
		{
			g_free(arc_path);
			g_match_info_next(data->match_info, NULL);
			return E_SUCCESS;
		}

		gchar *args = g_strdup_printf("zpaq\nx\n%s\n%s\n-all\n%s\n-threads\n%s\n-to\n%s\n-f", data->arc, arc_path, gOptVerNum, gOptThreads, DestName);

		if (gPass[0] != '\0')
		{
			gchar *tmp = g_strdup_printf("%s\n-key\n%s", args, gPass);
			g_free(args);
			args = tmp;
		}

		gchar **argv = g_strsplit(args, "\n", -1);
		g_free(args);
		result = execute_archiver(NULL, argv, DestName, data->ProcessDataProc);
		g_strfreev(argv);
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

	if (data->re_info)
		g_regex_unref(data->re_info);

	g_match_info_free(data->match_info);
	g_regex_unref(data->re);
	g_free(data->output);
	g_free(data->arc);
	g_free(data);

	return E_SUCCESS;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	int result = E_SUCCESS;
	gchar *filelist = AddList;
	gchar *arcdir = NULL;

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

	gchar *args = g_strdup_printf("zpaq\na\n%s", PackedFile);

	while (*filelist)
	{
		gchar *tmp = g_strdup_printf("%s\n%s", args, filelist);
		g_free(args);
		args = tmp;

		while (*filelist++);
	}

	if (arcdir)
	{
		filelist = AddList;
		gchar *tmp = g_strdup_printf("%s\n-to", args);
		g_free(args);
		args = tmp;

		while (*filelist)
		{
			gchar *tmp = g_strdup_printf("%s\n%s/%s", args, arcdir, filelist);
			g_free(args);
			args = tmp;

			while (*filelist++);
		}
	}

	gchar *tmp = g_strdup_printf("%s\n-m%s\n-t%s\n-f", args, gOptMethod, gOptThreads);
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

	result = execute_archiver(SrcPath, argv, PackedFile, gProcessDataProc);
	g_strfreev(argv);

	gProcessDataProc(PackedFile, -100);
	g_free(arcdir);

	return result;
}

int DCPCALL DeleteFiles(char *PackedFile, char *DeleteList)
{
	int result = E_SUCCESS;
	char filename[PATH_MAX];
	gchar *filelist = DeleteList;

	if (strcmp(gOptVerNum, "0") != 0)
		return E_NOT_SUPPORTED;

	if (strcmp(gPassLastFile, PackedFile) != 0)
		gPass[0] = '\0';

	gchar *args = g_strdup_printf("zpaq\na\n%s", PackedFile);

	while (*filelist)
	{
		gchar *tmp = g_strdup_printf("%s\n/dev/null", args);
		g_free(args);
		args = tmp;

		while (*filelist++);
	}

	gchar *tmp = g_strdup_printf("%s\n-to", args);
	g_free(args);
	args = tmp;
	filelist = DeleteList;

	while (*filelist)
	{
		char mask[5];
		size_t len = strlen(DeleteList);
		g_strlcpy(mask, filelist + (len - 4), 5);

		if (strcmp(mask, "/*.*") == 0)
			g_strlcpy(filename, filelist, len - 3);
		else
			g_strlcpy(filename, filelist, PATH_MAX);

		gchar *tmp = g_strdup_printf("%s\n%s", args, filename);
		g_free(args);
		args = tmp;

		while (*filelist++);
	}

	tmp = g_strdup_printf("%s\n-m%s\n-t%s", args, gOptMethod, gOptThreads);
	g_free(args);

	if (gPass[0] != '\0')
	{
		args = g_strdup_printf("%s\n-key\n%s-f", tmp, gPass);
		g_free(tmp);
	}
	else
		args = tmp;

	gchar **argv = g_strsplit(args, "\n", -1);
	g_free(args);

	result = execute_archiver(NULL, argv, PackedFile, gProcessDataProc);
	g_strfreev(argv);

	gProcessDataProc(PackedFile, -100);

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
	return PK_CAPS;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	FILE *fp = fopen(FileName, "r");

	if (!fp)
		return FALSE;

	char head_block[HEADER_SIZE];
	fread(head_block, 1, HEADER_SIZE, fp);
	fclose(fp);

	if (strcmp(head_block, HEADER) == 0)
		return TRUE;

	return FALSE;
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
	                         "  object chSkipInfo: TCheckBox\n"
	                         "    AnchorSideLeft.Control = chAll\n"
	                         "    AnchorSideTop.Control = edDigits\n"
	                         "    AnchorSideTop.Side = asrBottom\n"
	                         "    Left = 20\n"
	                         "    Height = 23\n"
	                         "    Top = 48\n"
	                         "    Width = 277\n"
	                         "    Caption = 'Exclude information entities from the filelist'\n"
	                         "    Checked = True\n"
	                         "    Enabled = False\n"
	                         "    State = cbChecked\n"
	                         "    TabOrder = 2\n"
	                         "  end\n"
	                         "  object gbCompr: TGroupBox\n"
	                         "    AnchorSideLeft.Control = Owner\n"
	                         "    AnchorSideTop.Control = chSkipInfo\n"
	                         "    AnchorSideTop.Side = asrBottom\n"
	                         "    AnchorSideRight.Control = edDigits\n"
	                         "    AnchorSideRight.Side = asrBottom\n"
	                         "    Left = 20\n"
	                         "    Height = 96\n"
	                         "    Top = 91\n"
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
	                         "    TabOrder = 3\n"
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
	                         "    Top = 217\n"
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
	                         "    TabOrder = 5\n"
	                         "  end\n"
	                         "  object btnOK: TBitBtn\n"
	                         "    AnchorSideTop.Control = gbCompr\n"
	                         "    AnchorSideTop.Side = asrBottom\n"
	                         "    AnchorSideRight.Control = gbCompr\n"
	                         "    AnchorSideRight.Side = asrBottom\n"
	                         "    Left = 243\n"
	                         "    Height = 30\n"
	                         "    Top = 217\n"
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
	                         "    TabOrder = 4\n"
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
