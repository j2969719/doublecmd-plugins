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
#define SendDlgMsg gDialogApi->SendDlgMsg

typedef struct sVFSDirData
{
	DIR *cur;
} tVFSDirData;

typedef struct sStatusLines
{
	char *name;
	char *control;
	int  type;
} tStatusLines;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;

static char gLFMPath[PATH_MAX];
static char gLastPath[PATH_MAX];
static char gLinkPath[PATH_MAX] = "status";

tStatusLines gStatusLines[] =
{
	{"Name:",			"edName", 	    ft_string},
	{"Pid:",			"edPid",	ft_numeric_32},
	{"PPid:",			"edPpid",	ft_numeric_32},
	{"State:",			"lblState",	    ft_string},
	{"VmSize:",			"edVmSize",	    ft_string},
	{"VmPeak:",			"edVmPeak",	    ft_string},
	{"Umask:",			NULL,		    ft_string},
	{"Tgid:",			NULL,		ft_numeric_32},
	{"Ngid:",			NULL,		ft_numeric_32},
	{"TracerPid:",			NULL,		ft_numeric_32},
	{"Uid:",			NULL,		    ft_string},
	{"Gid:",			NULL,		    ft_string},
	{"FDSize:",			NULL,		ft_numeric_32},
	{"Groups:",			NULL,		    ft_string},
	{"NStgid:",			NULL,		ft_numeric_32},
	{"NSpid:",			NULL,		ft_numeric_32},
	{"NSpgid:",			NULL,		ft_numeric_32},
	{"NSsid:",			NULL,		ft_numeric_32},
	{"VmLck:",			"edVmLck",	    ft_string},
	{"VmPin:",			NULL,		    ft_string},
	{"VmHWM:",			"edVmHWM",	    ft_string},
	{"VmRSS:",		 	"edVmRSS",	    ft_string},
	{"RssAnon:",			NULL,		    ft_string},
	{"RssFile:",			NULL,		    ft_string},
	{"RssShmem:",			NULL,		    ft_string},
	{"VmData:",			NULL,		    ft_string},
	{"VmStk:",			NULL,		    ft_string},
	{"VmExe:",			NULL,		    ft_string},
	{"VmLib:",			NULL,		    ft_string},
	{"VmPTE:",			NULL,		    ft_string},
	{"VmSwap:",		  	"edVmSwap",	    ft_string},
	{"HugetlbPages:",		NULL,		    ft_string},
	{"CoreDumping:",		NULL,		    ft_string},
	{"THP_enabled:",		NULL,		    ft_string},
	{"Threads:",			NULL,		ft_numeric_32},
	{"SigQ:",			NULL,		    ft_string},
	{"SigPnd:",			NULL,		    ft_string},
	{"ShdPnd:",			NULL,		    ft_string},
	{"SigBlk:",			NULL,		    ft_string},
	{"SigIgn:",			NULL,		    ft_string},
	{"SigCgt:",			NULL,		    ft_string},
	{"CapInh:",			NULL,		    ft_string},
	{"CapPrm:",			NULL,		    ft_string},
	{"CapEff:",			NULL,		    ft_string},
	{"CapBnd:",			NULL,		    ft_string},
	{"CapAmb:",			NULL,		    ft_string},
	{"NoNewPrivs:",			NULL,		    ft_string},
	{"Seccomp:",			NULL,		    ft_string},
	{"Speculation_Store_Bypass:",	NULL,		    ft_string},
	{"Cpus_allowed:",		NULL,		    ft_string},
	{"Cpus_allowed_list:",		NULL,		    ft_string},
	{"Mems_allowed:",		NULL,		    ft_string},
	{"Mems_allowed_list:",		NULL,		    ft_string},
	{"voluntary_ctxt_switches:",	NULL,		    ft_string},
	{"nonvoluntary_ctxt_switches:",	NULL,		    ft_string},
};

#define statuslcount (sizeof(gStatusLines)/sizeof(tStatusLines))


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
				snprintf(FindData->cFileName, MAX_PATH, "%s.%s", name, ent->d_name);
				fclose(info);
				found = true;

				snprintf(lpath, PATH_MAX, "/proc/%s/exe", ent->d_name);
				FindData->dwFileAttributes = FILE_ATTRIBUTE_UNIX_MODE;

				if (access(lpath, F_OK) != 0)
					FindData->dwReserved0 = S_IRUSR;
				else
					FindData->dwReserved0 = S_IRUSR | S_IWUSR;

				snprintf(lpath, PATH_MAX, "/proc/%s/statm", ent->d_name);
				info = fopen(lpath, "r");

				if (info)
				{
					int vmsize, rssize;
					int64_t size = 0;
					fscanf(info, "%d %d ", &vmsize, &rssize);

					if (rssize > 0)
					{
						size = (int64_t)rssize * (int64_t)sysconf(_SC_PAGESIZE);
						FindData->nFileSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
						FindData->nFileSizeLow = size & 0x00000000FFFFFFFF;
					}
					else
					{
						FindData->nFileSizeHigh = 0xFFFFFFFF;
						FindData->nFileSizeLow = 0xFFFFFFFE;
					}

					fclose(info);
				}

				UnixTimeToFileTime(time(0), &FindData->ftCreationTime);
				UnixTimeToFileTime(time(0), &FindData->ftLastAccessTime);
				UnixTimeToFileTime(time(0), &FindData->ftLastWriteTime);
			}
		}
	}

	return found;
}

static void GetProcStatus(uintptr_t pDlg)
{
	char lpath[PATH_MAX];
	FILE *info;
	char *line = NULL;
	size_t len = 0;
	ssize_t lread;

	snprintf(lpath, PATH_MAX, "/proc/%s/status", gLastPath);

	if ((info = fopen(lpath, "r")) != NULL)
	{
		while ((lread = getline(&line, &len, info)) != -1)
		{
			if (line[lread - 1] == '\n')
				line[lread - 1] = '\0';

			for (int i = 0; i < statuslcount; i++)
			{
				if (gStatusLines[i].control && strncmp(line, gStatusLines[i].name, strlen(gStatusLines[i].name)) == 0)
				{
					SendDlgMsg(pDlg, gStatusLines[i].control, DM_SETTEXT, (intptr_t)line + strlen(gStatusLines[i].name) + 1, 0);
					break;
				}
			}
		}

		free(line);
		fclose(info);
	}
	else
	{
		for (int i = 0; i < statuslcount; i++)
		{
			if (gStatusLines[i].control)
				SendDlgMsg(pDlg, gStatusLines[i].control, DM_SETTEXT, (intptr_t)"N/A", 0);
		}
	}
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
		GetProcStatus(pDlg);
		snprintf(lpath, PATH_MAX, "/proc/%s/cmdline", gLastPath);
		int fd;

		if ((fd = open(lpath, O_RDONLY)) > 0)
		{
			memset(lpath, 0, PATH_MAX);

			if ((lread = read(fd, lpath, PATH_MAX - 1)) > 0)
			{
				for (int i = 0; i < lread - 1; i++)
				{
					if (lpath[i] == '\0')
						lpath[i] = ' ';
				}

				SendDlgMsg(pDlg, "mCmdline", DM_SETTEXT, (intptr_t)lpath, 0);
			}

			close(fd);
		}

		SendDlgMsg(pDlg, "cbLink", DM_SETTEXT, (intptr_t)gLinkPath, 0);

		break;

	case DN_TIMER:
		GetProcStatus(pDlg);

		break;
	case DN_CHANGE:
		if (strcmp(DlgItemName, "edPpid") == 0)
		{
			line = (char*)SendDlgMsg(pDlg, "edPpid", DM_GETTEXT, 0, 0);

			if (line && strcmp(line, "N/A") != 0)
			{
				snprintf(lpath, PATH_MAX, "/proc/%s/status", line);

				if ((info = fopen(lpath, "r")) != NULL)
				{
					char name[100];
					fscanf(info, "Name:\t%100[^\n]s", name);
					SendDlgMsg(pDlg, "edParentName", DM_SETTEXT, (intptr_t)name, 0);
					fclose(info);
				}
			}
		}
		else if (strncmp(DlgItemName, "edVm", 4) == 0)
		{
			line = (char*)SendDlgMsg(pDlg, DlgItemName, DM_GETTEXT, 0, 0);
			len = strlen(line);

			if (len > 7 && len + 2 < PATH_MAX)
			{
				if (len > 7 && line[len - 3] == ' ' && line[len - 7] != ' ')
				{
					snprintf(lpath, len - 6, "%s", line);
					strcat(lpath, " ");
					strcat(lpath, line + len - 6);
					SendDlgMsg(pDlg, DlgItemName, DM_SETTEXT, (intptr_t)lpath, 0);
				}
				else if (len > 11 && line[len - 3] == ' ' && line[len - 11] != ' ')
				{
					snprintf(lpath, len - 10, "%s", line);
					strcat(lpath, " ");
					strcat(lpath, line + len - 10);
					SendDlgMsg(pDlg, DlgItemName, DM_SETTEXT, (intptr_t)lpath, 0);
				}
			}
		}
		else if (strcmp(DlgItemName, "cbLink") == 0)
		{
			snprintf(gLinkPath, PATH_MAX, "%s", (char*)SendDlgMsg(pDlg, DlgItemName, DM_GETTEXT, 0, 0));
		}
		else if (strcmp(DlgItemName, "edUpdTime") == 0)
		{
			line = (char*)SendDlgMsg(pDlg, "edUpdTime", DM_GETTEXT, 0, 0);
			int interval = atoi(line);
			SendDlgMsg(pDlg, "tmUpdate", DM_TIMERSETINTERVAL, interval, 0);
		}

		break;
	}

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
				strcpy(gLastPath, "self");
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
					snprintf(gLastPath, PATH_MAX, "%s", dot + 1);
					gDialogApi->DialogBoxLFMFile(gLFMPath, DlgProc);
				}

				return FS_EXEC_OK;
			}
		}
	}
	else if (strncmp(Verb, "quote", 5) == 0)
	{
		snprintf(RemoteName, MAX_PATH, "/proc/%s/", Verb + 6);

		if (access(RemoteName, F_OK) == 0)
			return FS_EXEC_SYMLINK;
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
		snprintf(RemoteName, maxlen - 1, "/proc/%s/%s", dot + 1, gLinkPath);
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

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= statuslcount)
		return ft_nomorefields;

	int len = strlen(gStatusLines[FieldIndex].name);

	if (len < 0 || len > maxlen)
		return ft_nomorefields;

	strncpy(FieldName, gStatusLines[FieldIndex].name, maxlen - 1);

	if (FieldName[len - 1] == ':')
		FieldName[len - 1] = '\0';

	Units[0] = '\0';
	return gStatusLines[FieldIndex].type;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int result = ft_fieldempty;
	char lpath[PATH_MAX];
	FILE *info;
	char *line = NULL;
	size_t len = 0;
	ssize_t lread;

	char *dot = strrchr(FileName, '.');

	if (dot != NULL)
	{
		snprintf(lpath, PATH_MAX, "/proc/%s/status", dot + 1);

		if ((info = fopen(lpath, "r")) != NULL)
		{
			while ((lread = getline(&line, &len, info)) != -1)
			{
				if (line[lread - 1] == '\n')
					line[lread - 1] = '\0';

				if (strncmp(line, gStatusLines[FieldIndex].name, strlen(gStatusLines[FieldIndex].name)) == 0)
				{
					if (line + strlen(gStatusLines[FieldIndex].name) + 1)
					{
						if (gStatusLines[FieldIndex].type == ft_string)
							strncpy((char*)FieldValue, line + strlen(gStatusLines[FieldIndex].name) + 1, maxlen - 1);
						else if (gStatusLines[FieldIndex].type == ft_numeric_32)
							*(int*)FieldValue = atoi(line + strlen(gStatusLines[FieldIndex].name) + 1);

						result = gStatusLines[FieldIndex].type;
					}

					break;
				}
			}

			free(line);
			fclose(info);
		}
		else
			result = ft_fileerror;
	}
	else
		result = ft_fileerror;

	return result;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	strncpy(ViewContents, "[Plugin(FS).State{}]\\n[DC().GETFILESIZE{}]\\n[Plugin(FS).Pid{}]\\n[Plugin(FS).Threads{}]", maxlen - 1);
	strncpy(ViewHeaders, "State\\nMem\\nPid\\nThreads", maxlen - 1);
	strncpy(ViewWidths, "100,0,55,-40,-30,-35", maxlen - 1);
	strncpy(ViewOptions, "-1|0", maxlen - 1);
	return true;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	strncpy(DefRootName, "Process List", maxlen - 1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gDialogApi == NULL)
	{
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));

		Dl_info dlinfo;
		memset(&dlinfo, 0, sizeof(dlinfo));

		if (dladdr(gLFMPath, &dlinfo) != 0)
		{
			strncpy(gLFMPath, dlinfo.dli_fname, PATH_MAX);
			char *pos = strrchr(gLFMPath, '/');

			if (pos)
			{
				if (gDialogApi && gDialogApi->VersionAPI > 0)
					strcpy(pos + 1, "dialog_with_timer.lfm");
				else
					strcpy(pos + 1, "dialog.lfm");
			}
		}
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gDialogApi != NULL)
		free(gDialogApi);

	gDialogApi = NULL;
}
