#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include "wfxplugin.h"


#define _plugname "FileList"
#define _filename "filelist.txt"
#define _inFile "/tmp/doublecmd-filelist.txt"
#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
FILE *fp;
char inFile[PATH_MAX];

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

bool getFileFromList(FILE *List, WIN32_FIND_DATAA *FindData)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	struct stat buf;
	bool found = false;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while ((read = getline(&line, &len, fp)) != -1)
	{
		if (line[read - 1] == '\n')
			line[read - 1] = '\0';

		if ((stat(line, &buf) == 0) && S_ISREG(buf.st_mode))
		{
			found = true;
			break;
		}
	}

	if (found)
	{
		FindData->nFileSizeLow = buf.st_size;
		strlcpy(FindData->cFileName, line, PATH_MAX);
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
/*
		FindData->dwFileAttributes = 0x80000000;
		FindData->dwReserved0 = buf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX);
		*/
		UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
		UnixTimeToFileTime(buf.st_atime, &FindData->ftLastAccessTime);
		FindData->dwFileAttributes = 0;
		return true;
	}

	return false;
}

bool inFileList(char *filename)
{
	FILE *tfp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	tfp = fopen(inFile, "r");

	if (tfp)
	{
		while ((read = getline(&line, &len, tfp)) != -1)
		{
			if (line[read - 1] == '\n')
				line[read - 1] = '\0';

			if (strcmp(line, filename) == 0)
			{
				fclose(tfp);
				return true;
			}
		}

		fclose(tfp);
	}

	return false;
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

	fp = fopen(inFile, "r");

	if ((fp != NULL) && (getFileFromList(fp, FindData)))
		return (HANDLE)(1985);

	return (HANDLE)(-1);
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	if (getFileFromList(fp, FindData))
		return true;

	return false;
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	fclose(fp);
	return 0;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	FILE *ifp, *ofp;
	char *line = NULL;
	char tname[PATH_MAX + 1];
	char tmpFile[PATH_MAX];
	size_t len = 0;

	ifp = fopen(inFile, "r");

	if (ifp)
	{
		snprintf(tmpFile, PATH_MAX, "%s.tmp", inFile);
		ofp = fopen(tmpFile, "w");

		if (ofp)
		{
			while (getline(&line, &len, ifp) != -1)
			{
				strlcpy(line, line, PATH_MAX);
				snprintf(tname, PATH_MAX + 1, "/%s", line);
				strlcpy(tname, tname, strlen(tname) - 1);

				if (strcmp(tname, RemoteName) != 0)
				{
					fprintf(ofp, line);
				}
			}

			fclose(ofp);
		}

		fclose(ifp);
	}

	rename(tmpFile, inFile);
	return true;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return true;

}

BOOL DCPCALL FsMkDir(char* Path)
{
	return true;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);

	if (err)
		return FS_FILE_USERABORT;

	if ((CopyFlags == 0) && (inFileList(LocalName)))
	{
		gProgressProc(gPluginNr, RemoteName, LocalName, 100);
		return FS_FILE_OK;
	}

	FILE *tfp;

	tfp = fopen(inFile, "a");

	if (tfp)
	{
		fprintf(tfp, "%s\n", LocalName);
		fclose(tfp);
		gProgressProc(gPluginNr, RemoteName, LocalName, 100);
		return FS_FILE_OK;
	}

	return FS_FILE_WRITEERROR;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strcmp(Verb, "open") == 0)
	{
//		return FS_EXEC_YOURSELF;
		char *command = malloc(strlen(RemoteName) + 12);;
		sprintf(command, "xdg-open \"%s\"", RemoteName + 1);
		system(command);
		free(command);
	}

	return FS_EXEC_OK;
}


void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	strlcpy(inFile, dps->DefaultIniName, PATH_MAX - 1);
	char *pos = strrchr(inFile, '/');

	if (pos)
		strcpy(pos + 1, _filename);
	else
		strlcpy(inFile, _inFile, PATH_MAX);
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	strlcpy(DefRootName, _plugname, maxlen - 1);
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	strlcpy(RemoteName, RemoteName + 1, maxlen - 1);
	return true;
}
