#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

typedef void *HINSTANCE;

#define FALSE 0
#define TRUE !FALSE

#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define BUMP_TIME(A, B) ((gOptTime == OPT_TIME_OLDEST && A < B) || (gOptTime == OPT_TIME_NEWWEST && A > B))

enum
{
	OPT_TIME_NEWWEST,
	OPT_TIME_OLDEST,
	OPTS_TIME
};

enum
{
	OPT_TYPE_REG,
	OPT_TYPE_DIRLNK,
	OPT_TYPE_ALLLNK,
	OPT_TYPE_LNKSELF,
	OPT_TYPE_ALL,
	OPTS_TYPE
};

enum
{
	OPT_DIR_IGNORE,
	OPT_DIR_EMPTY,
	OPT_DIR_ALL,
	OPTS_DIR
};

int gOptTime = 0; 
int gOptType = 0; 
int gOptDirs = 0;
BOOL gOptOnlyRoot = FALSE;
tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;
static char gLFMPath[EXT_MAX_PATH + 12] = "";

intptr_t DCPCALL OptionsDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		SendDlgMsg(pDlg, "cbTime", DM_LISTSETITEMINDEX, (intptr_t)gOptTime, 0);
		SendDlgMsg(pDlg, "cbType", DM_LISTSETITEMINDEX, (intptr_t)gOptType, 0);
		SendDlgMsg(pDlg, "cbDirs", DM_LISTSETITEMINDEX, (intptr_t)gOptDirs, 0);
		SendDlgMsg(pDlg, "lblDirs", DM_ENABLE, (gOptType == OPT_TYPE_ALL), 0);
		SendDlgMsg(pDlg, "cbDirs", DM_ENABLE, (gOptType == OPT_TYPE_ALL), 0);
		SendDlgMsg(pDlg, "chOnlyRoot", DM_SETCHECK, (intptr_t)gOptOnlyRoot, 0);
		break;
	case DN_CHANGE:
		if (strcmp(DlgItemName, "cbTime") == 0)
			gOptTime = (int)SendDlgMsg(pDlg, "cbTime", DM_LISTGETITEMINDEX, 0, 0);
		else if (strcmp(DlgItemName, "cbType") == 0)
		{
			gOptType = (int)SendDlgMsg(pDlg, "cbType", DM_LISTGETITEMINDEX, 0, 0);
			SendDlgMsg(pDlg, "lblDirs", DM_ENABLE, (gOptType == OPT_TYPE_ALL), 0);
			SendDlgMsg(pDlg, "cbDirs", DM_ENABLE, (gOptType == OPT_TYPE_ALL), 0);
		}
		else if (strcmp(DlgItemName, "cbDirs") == 0)
			gOptDirs = (int)SendDlgMsg(pDlg, "cbDirs", DM_LISTGETITEMINDEX, 0, 0);
		else if (strcmp(DlgItemName, "chOnlyRoot") == 0)
			gOptOnlyRoot = (wParam != FALSE);
		break;
	}
	return 0;
}

static BOOL SetTime(const char *path, time_t *unixtime);
static BOOL CheckTime(char *file, unsigned char d_type, time_t *value)
{
	struct stat buf;

	if (gOptType < OPT_TYPE_ALL && d_type != DT_REG && d_type != DT_LNK)
		return FALSE;

	if (gOptType == OPT_TYPE_REG && d_type != DT_REG)
		return FALSE;

	if (stat(file, &buf) == 0)
	{
		if (d_type == DT_LNK && gOptType == OPT_TYPE_LNKSELF && lstat(file, &buf) != 0)
			return FALSE;
		else if (d_type == DT_LNK && S_ISDIR(buf.st_mode))
			return SetTime(file, value);
		else if (d_type == DT_LNK && gOptType < OPT_TYPE_ALLLNK)
			return FALSE;
		else if (BUMP_TIME(buf.st_mtime, *value))
			*value = buf.st_mtime;
	}

	return TRUE;
}

static BOOL SetTime(const char *path, time_t *unixtime)
{
	DIR *dir;
	struct dirent *ent;
	char file[PATH_MAX];
	time_t value = -1;
	BOOL result = FALSE;
	char frmt_path[] = "%s/%s";

	if (path[strlen(path) - 1] == '/')
		strcpy(frmt_path, "%s%s");

	if (gOptTime == OPT_TIME_OLDEST)
		value = time(0);

	if ((dir = opendir(path)) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
			{
				snprintf(file, PATH_MAX, frmt_path, path, ent->d_name);

				if (ent->d_type != DT_DIR)
				{
					if (CheckTime(file, ent->d_type, &value))
						result = TRUE;
				}
				else
				{
					if (gOptDirs == OPT_DIR_ALL && CheckTime(file, ent->d_type, &value))
						result = TRUE;

					if (access(file, R_OK | W_OK | X_OK) == 0)
					{
						if (SetTime(file, &value))
							result = TRUE;
						else if (gOptDirs == OPT_DIR_EMPTY && CheckTime(file, ent->d_type, &value))
							result = TRUE;
					}
					else
						printf("%s: cant access %s\n", PLUGNAME, file);
				}
			}
		}

		closedir(dir);
	}


	if (result)
	{
		if (!gOptOnlyRoot || (gOptOnlyRoot && unixtime == NULL))
		{
			struct utimbuf ubuf;
			ubuf.actime = time(0);
			ubuf.modtime = value;
			utime(path, &ubuf);
		}

		if (unixtime != NULL && BUMP_TIME(value, *unixtime))
			*unixtime = value;
	}

	return result;
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
	return E_SUCCESS;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	int len = 0;
	char root[PATH_MAX] = "";
	char path[PATH_MAX] = "";

	while (*AddList)
	{
		if (AddList[strlen(AddList) - 1] == '/')
		{
			snprintf(path, PATH_MAX, "%s%s", SrcPath, AddList);

			if (gProcessDataProc(path, 0)== 0)
				return E_EABORTED;

			if (len == 0 || strncmp(root, path, len) != 0)
			{
				strcpy(root, path);
				len = strlen(root);
				if (access(root, R_OK | W_OK | X_OK) != 0)
				{
					snprintf(path, PATH_MAX, "u cant touch this %s", AddList);

					if (MessageBox(path, PLUGNAME, MB_OKCANCEL | MB_ICONERROR) == ID_CANCEL)
						return E_EABORTED;
				}
				else
					SetTime(root, NULL);
			}
		}

		while (*AddList++);
	}

	return E_SUCCESS;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	gProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{
	return;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_HIDE | PK_CAPS_OPTIONS;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	return FALSE;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	if (access(gLFMPath, F_OK) != 0)
		MessageBox("dialog.lfm is missing!", PLUGNAME, MB_OK | MB_ICONERROR);
	else
		gExtensions->DialogBoxLFMFile(gLFMPath, OptionsDlgProc);
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
}

