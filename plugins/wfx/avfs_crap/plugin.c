#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <virtual.h>
#include <string.h>
#include <time.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))

typedef struct sAVFSDirData
{
	DIR *cur;
	char path[PATH_MAX];
} tAVFSDirData;

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
tExtensionStartupInfo* gStartupInfo;

static char gAVFSPath[PATH_MAX] = "/#ftp:ftp.funet.fi/pub/Linux";

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

bool SetFindData(DIR *cur, char *path, WIN32_FIND_DATAA *FindData)
{
	struct dirent *ent;
	struct stat buf;
	char lpath[PATH_MAX];

	ent = virt_readdir(cur);

	if (ent != NULL)
	{
		memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

		snprintf(lpath, sizeof(lpath), "%s/%s", path, ent->d_name);

		if (virt_stat(lpath, &buf) == 0)
		{
			if (S_ISDIR(buf.st_mode))
				FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

			FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;

			UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);

			FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
			FindData->dwReserved0 = buf.st_mode;
		}

		strlcpy(FindData->cFileName, ent->d_name, PATH_MAX - 1);

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
	tAVFSDirData *dirdata;
	dirdata = malloc(sizeof(tAVFSDirData));
	memset(dirdata, 0, sizeof(tAVFSDirData));

	snprintf(dirdata->path, sizeof(dirdata->path), "%s%s", gAVFSPath, Path);
	dirdata->cur = virt_opendir(dirdata->path);

	if (dirdata->cur != NULL && SetFindData(dirdata->cur, dirdata->path, FindData) == true)
		return (HANDLE)dirdata;

	return (HANDLE)(-1);
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tAVFSDirData *dirdata = (tAVFSDirData*)Hdl;

	return SetFindData(dirdata->cur, dirdata->path, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tAVFSDirData *dirdata = (tAVFSDirData*)Hdl;

	if (dirdata->cur != NULL)
		virt_closedir(dirdata->cur);

	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int ifd, ofd, done;
	ssize_t len, total = 0;
	char rpath[PATH_MAX];
	char buff[8192];
	int result = FS_FILE_OK;

	if ((CopyFlags == 0) && (access(LocalName, F_OK) == 0))
		return FS_FILE_EXISTS;

	snprintf(rpath, sizeof(rpath), "%s%s", gAVFSPath, RemoteName);

	if (gProgressProc(gPluginNr, rpath, LocalName, 0) == 1)
		return FS_FILE_USERABORT;

	ifd = virt_open(rpath, O_RDONLY, 0);

	if (ifd == -1)
		return FS_FILE_READERROR;

	ofd = open(LocalName, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

	if (ofd > -1)
	{
		size_t rsize = ((int64_t)ri->SizeHigh << 32) | ri->SizeLow;

		while ((len = virt_read(ifd, buff, sizeof(buff))) > 0)
		{
			if (write(ofd, buff, len) == -1)
				result = FS_FILE_WRITEERROR;

			total += len;

			if (rsize > 0)
				done = total * 100 / rsize;
			else
				done = 0;

			if (done > 100)
				done = 100;

			if (gProgressProc(gPluginNr, rpath, LocalName, done) == 1)
			{
				result = FS_FILE_USERABORT;
				break;
			}
		}

		close(ofd);
	}
	else
		result = FS_FILE_WRITEERROR;

	if (ri->Attr > 0)
		chmod(LocalName, ri->Attr);

	virt_close(ifd);

	return result;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strcmp(Verb, "open") == 0)
		return FS_EXEC_YOURSELF;

	return FS_EXEC_OK;
}

void DCPCALL FsStatusInfo(char* RemoteDir, int InfoStartEnd, int InfoOperation)
{
	if (strcmp(RemoteDir, "/") == 0)
	{
		if (InfoStartEnd == FS_STATUS_START && InfoOperation == FS_STATUS_OP_LIST)
			gRequestProc(gPluginNr, RT_TargetDir, "AVFS", "Enter AVFS path:", gAVFSPath, sizeof(gAVFSPath) - 1);
	}
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{

}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	strlcpy(DefRootName, "avfs-tst", maxlen - 1);
}

/*
void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	gStartupInfo = malloc(sizeof(tExtensionStartupInfo));
	memcpy(gStartupInfo, StartupInfo, sizeof(tExtensionStartupInfo));
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	free(gStartupInfo);
}
*/
