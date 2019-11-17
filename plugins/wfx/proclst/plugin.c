#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))

typedef struct sVFSDirData
{
	DIR *cur;
} tVFSDirData;

typedef struct sStatusLines
{
	char *name;
	char *control;
} tStatusLines;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;

static char gLFMPath[PATH_MAX];
static char gLastPath[PATH_MAX];

tStatusLines gStatusLines[] =
{
	{"Name:",	"edName"},
	{"Pid:",	"edPid"},
	{"PPid:",	"edPpid"},
	{"State:",	"lblState"},
	{"VmSize:",	"edVmSize"},
	{"VmPeak:",	"edVmPeak"},
};

#define statuslcount (sizeof(gStatusLines)/sizeof(tStatusLines))

char* strlcpy(char* p, const char* p2, int maxlen)
{
	if ((int)strlen(p2) >= maxlen)
	{
		strncpy(p, p2, maxlen);
		p[maxlen] = 0;
	}
	else
		strcpy(p, p2);

	return p;
}

void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	int64_t ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

bool SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	struct dirent *ent;
	bool found = false;
	FILE *info;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	char lpath[PATH_MAX];

	while (!found && (ent = readdir(dirdata->cur)) != NULL)
	{
		if (atoi(ent->d_name) > 0)
		{
			snprintf(lpath, PATH_MAX, "/proc/%s/status", ent->d_name);
			info = fopen(lpath, "r");

			if (info)
			{
				char name[100];
				fscanf(info, "Name:\t%100[^\n]s", name);
				snprintf(lpath, PATH_MAX, "%s.%s", name, ent->d_name);
				strlcpy(FindData->cFileName, lpath, MAX_PATH - 1);
				fclose(info);
				found = true;

				UnixTimeToFileTime(time(0), &FindData->ftCreationTime);
				UnixTimeToFileTime(time(0), &FindData->ftLastAccessTime);
				UnixTimeToFileTime(time(0), &FindData->ftLastWriteTime);

				snprintf(lpath, PATH_MAX, "/proc/%s/statm", ent->d_name);
				info = fopen(lpath, "r");

				if (info)
				{
					int64_t size = 0;
					fscanf(info, "%d ", &size);
					FindData->nFileSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
					FindData->nFileSizeLow = size & 0x00000000FFFFFFFF;
					fclose(info);
				}
			}
		}
	}

	return found;
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	char lpath[PATH_MAX];
	FILE *info;
	char *line = NULL;
	size_t len = 0;
	ssize_t lread;

	switch (Msg)
	{
	case DN_INITDIALOG:
		//printf("DlgProc(%s): DN_INITDIALOG stub\n", DlgItemName);

		snprintf(lpath, PATH_MAX, "/proc/%s/status", gLastPath);

		if ((info = fopen(lpath, "r")) != NULL)
		{
			while ((lread = getline(&line, &len, info)) != -1)
			{
				if (line[lread - 1] == '\n')
					line[lread - 1] = '\0';

				for (int i = 0; i < statuslcount; i++)
				{
					if (strncmp(line, gStatusLines[i].name, strlen(gStatusLines[i].name)) == 0)
					{
						gDialogApi->SendDlgMsg(pDlg, gStatusLines[i].control, DM_SETTEXT, (intptr_t)line + strlen(gStatusLines[i].name) + 1, 0);
						break;
					}
				}
			}

			fclose(info);
		}

		snprintf(lpath, PATH_MAX, "/proc/%s/cmdline", gLastPath);
		int fd;
		if ((fd = open(lpath, O_RDONLY)) > 0)
		{
			memset(lpath, 0, PATH_MAX);
			if ((lread = read(fd, lpath, PATH_MAX-1)) > 0)
			{
				for (int i=0; i<lread; i++)
				{
					if (lpath[i] == '\0')
						lpath[i] = ' ';
				}

				gDialogApi->SendDlgMsg(pDlg, "mCmdline", DM_SETTEXT, (intptr_t)lpath, 0);
			}
			close(fd);
		}

		break;

	case DN_CLOSE:
		printf("DlgProc(%s): DN_CLOSE stub\n", DlgItemName);

		break;

	case DN_CLICK:
		printf("DlgProc(%s): DN_CLICK stub\n", DlgItemName);

		break;

	case DN_DBLCLICK:
		printf("DlgProc(%s): DN_DBLCLICK stub\n", DlgItemName);

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "edPpid") == 0)
		{
			line = strdup((char*)gDialogApi->SendDlgMsg(pDlg, "edPpid", DM_GETTEXT, 0, 0));

			if (line && strcmp(line, "N/A") != 0)
			{
				snprintf(lpath, PATH_MAX, "/proc/%s/status", line);

				if ((info = fopen(lpath, "r")) != NULL)
				{
					char name[100];
					fscanf(info, "Name:\t%100[^\n]s", name);
					gDialogApi->SendDlgMsg(pDlg, "edParentName", DM_SETTEXT, (intptr_t)name, 0);
					fclose(info);
				}
			}
		}
		else

			printf("DlgProc(%s): DN_CHANGE stub\n", DlgItemName);

		break;

	case DN_GOTFOCUS:
		printf("DlgProc(%s): DN_GOTFOCUS stub\n", DlgItemName);

		break;

	case DN_KILLFOCUS:
		printf("DlgProc(%s): DN_KILLFOCUS stub\n", DlgItemName);

		break;

	case DN_KEYDOWN:
		printf("DlgProc(%s): DN_KEYDOWN stub\n", DlgItemName);

		break;

	case DN_KEYUP:
		printf("DlgProc(%s): DN_KEYUP stub\n", DlgItemName);

		break;
	}

	if (line)
		free(line);

	return 0;
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
	dirdata = malloc(sizeof(tVFSDirData));

	if (dirdata == NULL)
		return (HANDLE)(-1);

	memset(dirdata, 0, sizeof(tVFSDirData));

	if ((dirdata->cur = opendir("/proc/")) == NULL)
	{
		int errsv = errno;
		char msg[PATH_MAX];
		snprintf(msg, sizeof(msg), "/proc/: %s", strerror(errsv));
		gRequestProc(gPluginNr, RT_MsgOK, NULL, msg, NULL, 0);
	}

	if (dirdata->cur != NULL && SetFindData(dirdata, FindData) == true)
		return (HANDLE)dirdata;

	if (dirdata->cur != NULL)
		closedir(dirdata->cur);

	if (dirdata != NULL)
		free(dirdata);

	return (HANDLE)(-1);
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return SetFindData(dirdata, FindData);
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

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strcmp(Verb, "open") == 0 || strcmp(Verb, "properties") == 0)
	{
		if (RemoteName[1] == '\0')
		{
			if (gDialogApi && access(gLFMPath, F_OK) == 0)
			{
				strlcpy(gLastPath, "self", PATH_MAX);
				gDialogApi->DialogBoxLFMFile(gLFMPath, DlgProc);
			}

			return FS_EXEC_OK;
		}
		else
		{
			char *dot = strrchr(RemoteName, '.');

			if (dot != NULL)
			{
				if (gDialogApi && access(gLFMPath, F_OK) == 0)
				{
					strlcpy(gLastPath, dot + 1, PATH_MAX);
					gDialogApi->DialogBoxLFMFile(gLFMPath, DlgProc);
				}

				return FS_EXEC_OK;
			}
		}
	}


	return FS_EXEC_ERROR;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return true;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	char *dot = strrchr(RemoteName, '.');

	if (dot != NULL)
	{
		snprintf(RemoteName, maxlen - 1, "/proc/%s/status", strdup(dot + 1));
		return true;
	}

	return false;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	char *dot = strrchr(RemoteName, '.');

	if (dot != NULL)
	{
		int pid = atoi(dot + 1);

		if (kill((pid_t)pid, SIGKILL) == 0)
			return true;
	}

	return false;
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	const char* lfm_name = "dialog.lfm";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(lfm_name, &dlinfo) != 0)
	{
		strlcpy(gLFMPath, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(gLFMPath, '/');

		if (pos)
			strcpy(pos + 1, lfm_name);
	}
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	strlcpy(DefRootName, "Process List", maxlen - 1);
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
		free(gDialogApi);

	gDialogApi = NULL;
}
