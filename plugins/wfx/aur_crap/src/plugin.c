#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <linux/limits.h>
#include <time.h>
#include <string.h>
#include "wfxplugin.h"

#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;

clock_t start_t;

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
	int64_t size = 404;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while ((read = getline(&line, &len, List)) != -1)
	{
		if (line[0] == '#')
			continue;

		if (line[read - 1] == '\n')
			line[read - 1] = '\0';

		FindData->nFileSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
		FindData->nFileSizeLow = size & 0x00000000FFFFFFFF;
		snprintf(FindData->cFileName, MAX_PATH - 1, "%s.tar.gz", line);
		UnixTimeToFileTime(time(0), &FindData->ftCreationTime);
		UnixTimeToFileTime(time(0), &FindData->ftLastAccessTime);
		UnixTimeToFileTime(time(0), &FindData->ftLastWriteTime);

		return true;
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
	if (system("curl https://aur.archlinux.org/pkgbase.gz | gzip -cd > /tmp/doublecmd-aur.lst") != 0)
		return (HANDLE)(-1);

	start_t = clock();

	FILE *fp = fopen("/tmp/doublecmd-aur.lst", "r");

	if ((fp != NULL) && (getFileFromList(fp, FindData)))
		return (HANDLE)fp;

	return (HANDLE)(-1);
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	FILE *fp = (FILE*)Hdl;

	return getFileFromList(fp, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	FILE *fp = (FILE*)Hdl;
	fclose(fp);
	printf("--> FsFindFirst/FsFindClose = %.2f\n", (double)(clock() - start_t) / CLOCKS_PER_SEC);
	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	if ((CopyFlags == 0) && (access(LocalName, F_OK) == 0))
		return FS_FILE_EXISTS;

	char command[PATH_MAX];
	snprintf(command, PATH_MAX, "curl https://aur.archlinux.org/cgit/aur.git/snapshot%s > %s", RemoteName, LocalName);

	if (system(command) != 0)
		return FS_FILE_WRITEERROR;

	return FS_FILE_OK;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	snprintf(DefRootName, maxlen - 1, "AUR");
}
