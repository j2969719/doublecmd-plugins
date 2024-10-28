#define _GNU_SOURCE
#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
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
#define MSG_UNTIL "Rollback the archive to this version when adding these files?"
#define ERRFILE "<ERROR>"
#define HEADER "7kSt"
#define HEADER_SIZE 4
#define E_HANDLED -32769
#define PK_CAPS PK_CAPS_NEW | PK_CAPS_MODIFY | PK_CAPS_MULTIPLE | PK_CAPS_SEARCHTEXT |\
 PK_CAPS_DELETE | PK_CAPS_OPTIONS | PK_CAPS_ENCRYPT | PK_CAPS_BY_CONTENT;

#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox

typedef struct sArcData
{
	GRegex *re;
	gchar *arc;
	gchar **lines;
	guint cur;
	guint count;
	GRegex *re_info;
	tChangeVolProc ChangeVolProc;
	tProcessDataProc ProcessDataProc;
	char lastfile[PATH_MAX];
} tArcData;

typedef struct sProgressData
{
	gint stdout;
	int *progress;
	gboolean *is_reading;
} tProgressData;

typedef tArcData* ArcData;
typedef void *HINSTANCE;

tChangeVolProc gChangeVolProc  = NULL;
tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;
static char gOptVerNum[3] = "0";
static char gOptMethod[4] = "1";
static char gOptThreads[3] = "0";
static char gPass[MAX_PATH] = "";
static char gPassLastFile[PATH_MAX] = "";
static char gHistoryFile[PATH_MAX] = "";
static gchar *gOptWTF = NULL;
gboolean gOptSkipInfo = TRUE;

static void UpdatePreview(uintptr_t pDlg)
{
	int is_wtf = SendDlgMsg(pDlg, "cbWTF", DM_ENABLE, 1, 0);
	SendDlgMsg(pDlg, "cbWTF", DM_ENABLE, is_wtf, 0);
	char *value = (char*)SendDlgMsg(pDlg, "cbCompr", DM_GETTEXT, 0, 0);
	char type = value[0];
	char blocks[3] = "";
	int index = SendDlgMsg(pDlg, "cbBlock", DM_LISTGETITEMINDEX, 0, 0);

	if (index != -1)
		snprintf(blocks, 3, "%d", index);

	value = (char*)SendDlgMsg(pDlg, "cbWTF", DM_GETTEXT, 0, 0);

	if (value[0] == '\0')
		is_wtf = FALSE;

	gchar *line = g_strdup_printf("%c%s%s%s", type, blocks, is_wtf ? "." : "", is_wtf ? value : "");
	SendDlgMsg(pDlg, "lblPreview", DM_SETTEXT, (intptr_t)line, 0);
	g_free(line);
}

intptr_t DCPCALL OptDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	int index;
	char *value;
	FILE *fp;
	size_t len = 0;
	ssize_t read = 0;
	int i = 0;
	char *line = NULL;

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

		char compr = gOptMethod[0];

		if (isdigit(compr))
			index = (int)compr - 48;
		else if (compr == 'x')
			index = 6;
		else if (compr == 's')
			index = 7;

		SendDlgMsg(pDlg, "chWTF", DM_SETCHECK, (index < 6 && gOptWTF), 0);

		SendDlgMsg(pDlg, "cbCompr", DM_LISTSETITEMINDEX, (intptr_t)index, 0);

		if (strlen(gOptMethod) > 1)
		{
			index = atoi(gOptMethod + 1);
			SendDlgMsg(pDlg, "cbBlock", DM_LISTSETITEMINDEX, (intptr_t)index, 0);
		}

		if ((fp = fopen(gHistoryFile, "r")) != NULL)
		{
			while (((read = getline(&line, &len, fp)) != -1))
			{
				if (i > MAX_PATH)
					break;

				if (line[read - 1] == '\n')
					line[read - 1] = '\0';

				if (line != NULL && line[0] != '\0')
				{
					if (i == 0)
						SendDlgMsg(pDlg, "cbWTF", DM_SETTEXT, (intptr_t)line, 0);

					SendDlgMsg(pDlg, "cbWTF", DM_LISTADD, (intptr_t)line, 0);
					i++;
				}
			}

			free(line);
			fclose(fp);
		}

		if (gOptWTF)
		{
			SendDlgMsg(pDlg, "cbWTF", DM_ENABLE, 1, 0);
			SendDlgMsg(pDlg, "cbWTF", DM_SETTEXT, (intptr_t)gOptWTF + 1, 0);
		}

		line = g_strdup_printf("%s%s", gOptMethod, gOptWTF ? gOptWTF : "");
		SendDlgMsg(pDlg, "lblPreview", DM_SETTEXT, (intptr_t)line, 0);
		g_free(line);

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
			snprintf(gOptMethod, 2, "%d", index);

			if (index == 6)
				sprintf(gOptMethod, "x");
			else if (index == 7)
				sprintf(gOptMethod, "s");

			index = SendDlgMsg(pDlg, "cbBlock", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
				snprintf(gOptMethod + 1, 3, "%d", index);

			int is_wtf = SendDlgMsg(pDlg, "cbWTF", DM_ENABLE, 1, 0);
			SendDlgMsg(pDlg, "cbWTF", DM_ENABLE, is_wtf, 0);
			value = strdup((char*)SendDlgMsg(pDlg, "cbWTF", DM_GETTEXT, 0, 0));

			if (value[0] != '\0' && is_wtf)
			{
				if ((fp = fopen(gHistoryFile, "w")) != NULL)
				{
					fprintf(fp, "%s\n", value);
					size_t count = (size_t)SendDlgMsg(pDlg, "cbWTF", DM_LISTGETCOUNT, 0, 0);

					for (i = 0; i < count; i++)
					{
						if (i > MAX_PATH)
							break;

						line = (char*)SendDlgMsg(pDlg, "cbWTF", DM_LISTGETITEM, i, 0);

						if (line != NULL && line[0] != '\0' && (strcmp(value, line) != 0))
							fprintf(fp, "%s\n", line);
					}

					fclose(fp);
				}

				g_free(gOptWTF);

				gOptWTF = g_strdup_printf(".%s", value);
			}
			else
			{
				g_free(gOptWTF);
				gOptWTF = NULL;
			}

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
		else if (strcmp(DlgItemName, "chWTF") == 0)
		{
			SendDlgMsg(pDlg, "cbWTF", DM_ENABLE, wParam, 0);
			UpdatePreview(pDlg);
		}
		else if (strcmp(DlgItemName, "cbCompr") == 0)
		{
		/*
			if (wParam < 2)
				SendDlgMsg(pDlg, "cbBlock", DM_LISTSETITEMINDEX, 4, 0);
			else
				SendDlgMsg(pDlg, "cbBlock", DM_LISTSETITEMINDEX, 6, 0);
		*/

			int is_wtf = (wParam > 5 || SendDlgMsg(pDlg, "chWTF", DM_GETCHECK, 0, 0));
			SendDlgMsg(pDlg, "cbWTF", DM_ENABLE, is_wtf, 0);
			UpdatePreview(pDlg);
		}
		else if (strcmp(DlgItemName, "cbWTF") == 0)
		{
			UpdatePreview(pDlg);
		}
		else if (strcmp(DlgItemName, "cbBlock") == 0)
		{
			UpdatePreview(pDlg);
		}

		break;
	}

	return 0;
}

static int ParseUtcDate(char *string)
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

static gpointer ReadOutput(gpointer userdata)
{
	gsize len, term;
	gchar *line = NULL;
	tProgressData *data = (tProgressData*)userdata;

	GIOChannel *channel = g_io_channel_unix_new(data->stdout);

	while (G_IO_STATUS_NORMAL == g_io_channel_read_line(channel, &line, &len, &term, NULL))
	{

		if (line)
		{
			int num = atoi(line);

			if (num > 0 && num < 100)
				*data->progress = num;
		}

		g_free(line);
	}

	g_io_channel_shutdown(channel, TRUE, NULL);
	g_io_channel_unref(channel);
	*data->is_reading = FALSE;
	return NULL;
}

static int ExecuteArchiver(char *workdir, char **argv, char *filename, tProcessDataProc process, int bar)
{
	GPid pid;
	gint stdout;
	gint status = 0;
	GError *err = NULL;
	int result = E_SUCCESS;
	int flags = G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD;

	if (!g_spawn_async_with_pipes(workdir, argv, NULL, flags, NULL, NULL, &pid, NULL, &stdout, NULL, &err))
	{
		if (err)
			MessageBox(err->message, PLUGNAME,  MB_OK | MB_ICONERROR);

		result = E_EABORTED;
	}
	else
	{
		tProgressData *data = g_new0(tProgressData, 1);
		int progress = 0;
		gboolean is_running = TRUE;
		data->stdout = stdout;
		data->progress = &progress;
		data->is_reading = &is_running;

		GThread *thread = g_thread_new("ARCOP_PROGRESS", ReadOutput, data);

		while (is_running)
		{
			if (process(filename, bar - progress) == 0)
			{
				kill(pid, SIGTERM);
				result = E_EABORTED;
				break;
			}

			sleep(0.5);
		}

		g_thread_join(thread);
		g_thread_unref(thread);
		g_free(data);

		waitpid(pid, &status, 0);

		g_spawn_close_pid(pid);

		if (result == E_SUCCESS && WEXITSTATUS(status) != 0)
			result = E_EWRITE;
	}

	if (err)
		g_error_free(err);

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
		ArchiveData->OpenResult = E_HANDLED;
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
		data->lines = g_strsplit(stdout, "\n", -1);
		data->count = g_strv_length(data->lines);

		if (data->count < 3 || strncmp(data->lines[0], "zpaq v7.15 ", 11) != 0)
		{
			if (err)
				g_error_free(err);

			g_strfreev(data->lines);
			g_free(stdout);
			g_free(data);
			if (data->count < 3)
				ArchiveData->OpenResult = E_NO_FILES;
			else
			{
				MessageBox("Only zpaq v7.15 is supported!", PLUGNAME,  MB_OK | MB_ICONERROR);
				ArchiveData->OpenResult = E_HANDLED;
			}

			return E_SUCCESS;
		}

		data->re = g_regex_new(RE_LIST, 0, 0, &err);

		if (data->re)
		{
			for (guint i = 3; i < data->count; i++)
			{
				if (g_regex_match(data->re, data->lines[i], 0, NULL))
				{
					data->cur = i;
					break;
				}
			}
		}
		else
		{
			g_strfreev(data->lines);
			g_free(stdout);
			g_free(data);
			ArchiveData->OpenResult = E_HANDLED;
			MessageBox(err->message, PLUGNAME,  MB_OK | MB_ICONERROR);

			if (err)
				g_error_free(err);

			return E_SUCCESS;
		}

		data->arc = g_strdup(ArchiveData->ArcName);

		if (strcmp(gOptVerNum, "0") != 0)
			data->re_info = g_regex_new(RE_INFO, 0, 0, NULL);
	}
	else
	{
		if (err)
			g_error_free(err);

		g_free(data);
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

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
	GMatchInfo *match_info;

	while (data->cur < data->count - 3)
	{
		if (g_regex_match(data->re, data->lines[data->cur++], 0, &match_info) && g_match_info_matches(match_info))
		{
			string = g_match_info_fetch_named(match_info, "name");

			if (string)
			{
				if (data->re_info && gOptSkipInfo && g_regex_match(data->re_info, string, 0, NULL))
				{
					char *p = strrchr(string, '/');

					if (p)
						*p = '\0';

					g_strlcpy(HeaderDataEx->FileName, string, sizeof(HeaderDataEx->FileName) - 1);

					g_free(string);

					string = g_match_info_fetch_named(match_info, "date");

					if (string)
						HeaderDataEx->FileTime = ParseUtcDate(string);

					g_free(string);

					HeaderDataEx->FileAttr = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
					data->lastfile[0] = '\0';
					return E_SUCCESS;
				}

				if (string[0] != '/')
					g_strlcpy(HeaderDataEx->FileName, string, sizeof(HeaderDataEx->FileName) - 1);
				else
					g_strlcpy(HeaderDataEx->FileName, ERRFILE, sizeof(HeaderDataEx->FileName) - 1);

				g_strlcpy(data->lastfile, string, PATH_MAX);
			}
			else
				g_strlcpy(HeaderDataEx->FileName, ERRFILE, sizeof(HeaderDataEx->FileName) - 1);

			g_free(string);

			string = g_match_info_fetch_named(match_info, "attr");

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

			string = g_match_info_fetch_named(match_info, "size");

			if (string)
			{
				gdouble filesize = g_ascii_strtod(string, NULL);
				HeaderDataEx->UnpSizeHigh = ((int64_t)filesize & 0xFFFFFFFF00000000) >> 32;
				HeaderDataEx->UnpSize = (int64_t)filesize & 0x00000000FFFFFFFF;
			}

			g_free(string);

			HeaderDataEx->PackSizeHigh = 0xFFFFFFFF;
			HeaderDataEx->PackSize = 0xFFFFFFFE;

			string = g_match_info_fetch_named(match_info, "date");

			if (string)
				HeaderDataEx->FileTime = ParseUtcDate(string);

			g_free(string);
			g_match_info_free(match_info);

			return E_SUCCESS;
		}
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	int result = E_SUCCESS;
	ArcData data = (ArcData)hArcData;

	if (data->ProcessDataProc(DestName, -1000) == 0)
		result = E_EABORTED;

	else if (Operation != PK_SKIP)
	{
		gboolean is_test = (Operation == PK_TEST);

		if (data->lastfile[0] == '\0')
			return E_SUCCESS;

		gchar *args = g_strdup_printf("zpaq\nx\n%s\n%s\n-all\n%s\n-threads\n%s\n%s%s\n-f",
		                              data->arc, data->lastfile, gOptVerNum, gOptThreads,
		                              is_test ? "-test" : "-to\n", is_test ? "" : DestName);

		if (gPass[0] != '\0')
		{
			gchar *tmp = g_strdup_printf("%s\n-key\n%s", args, gPass);
			g_free(args);
			args = tmp;
		}

		gchar **argv = g_strsplit(args, "\n", -1);
		g_free(args);
		result = ExecuteArchiver(NULL, argv, data->lastfile, data->ProcessDataProc, -1000);
		g_strfreev(argv);
		data->ProcessDataProc(data->lastfile, -1100);
	}

	int progress = (int)(data->cur * 100 / data->count);
	data->ProcessDataProc(DestName, -progress);

	return result;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData data = (ArcData)hArcData;

	if (data->re_info)
		g_regex_unref(data->re_info);

	g_regex_unref(data->re);
	g_strfreev(data->lines);
	g_free(data->arc);
	g_free(data);

	return E_SUCCESS;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	int result = E_SUCCESS;
	gchar *filelist = AddList;
	gint version = -1;
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
			version = atoi(split[0]);
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

	if (arcdir && arcdir[0] != '\0')
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

	gchar *tmp = g_strdup_printf("%s\n-m%s%s\n-t%s\n-f", args, gOptMethod, gOptWTF ? gOptWTF : "", gOptThreads);
	g_free(args);

	if (gPass[0] != '\0')
	{
		args = g_strdup_printf("%s\n-key\n%s", tmp, gPass);
		g_free(tmp);
	}
	else
		args = tmp;

	if (version > 0 && MessageBox(MSG_UNTIL, PLUGNAME, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == ID_YES)
	{
		gchar *tmp = g_strdup_printf("%s\n-until\n%d", args, version);
		g_free(args);
		args = tmp;
	}

	gchar **argv = g_strsplit(args, "\n", -1);
	g_free(args);

	result = ExecuteArchiver(SrcPath, argv, SrcPath, gProcessDataProc, 0);
	g_strfreev(argv);

	gProcessDataProc(SrcPath, -100);
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

	tmp = g_strdup_printf("%s\n-m%s%s\n-t%s", args, gOptMethod, gOptWTF ? gOptWTF : "", gOptThreads);
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

	result = ExecuteArchiver(NULL, argv, PackedFile, gProcessDataProc, 0);
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
	FILE *fp = fopen(FileName, "rb");

	if (!fp)
		return FALSE;

	char head_block[HEADER_SIZE];
	fread(head_block, 1, HEADER_SIZE, fp);
	fclose(fp);

	if (memcmp(head_block, HEADER, HEADER_SIZE) == 0)
		return TRUE;

	return FALSE;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	gchar *lfm_file = g_strdup_printf("%s/dialog.lfm", gExtensions->PluginDir);
	gExtensions->DialogBoxLFMFile(lfm_file, OptDlgProc);
	g_free(lfm_file);
}

void DCPCALL PackSetDefaultParams(PackDefaultParamStruct* dps)
{
	gchar *cfg_dir = g_path_get_dirname(dps->DefaultIniName);
	gchar *filename = g_strdup_printf("%s/history_%s.txt", cfg_dir, PLUGNAME);
	g_strlcpy(gHistoryFile, filename, PATH_MAX);
	g_free(cfg_dir);
	g_free(filename);
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

	if (gOptWTF != NULL)
		g_free(gOptWTF);

	gOptWTF = NULL;
}
