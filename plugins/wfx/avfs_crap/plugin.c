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

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
tExtensionStartupInfo* gStartupInfo;

//static char base[PATH_MAX + 1] = "/home/user/tst.diff#patchfs";
static char base[PATH_MAX + 1] = "/#ftp:ftp.funet.fi/pub/Linux";
static char path[PATH_MAX + 1];

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

bool SetFindData(DIR *cur, WIN32_FIND_DATAA *FindData)
{
	struct dirent *ent;
	struct stat buf;
	char lpath[PATH_MAX + 1];

	ent = virt_readdir(cur);

	if (ent != NULL)
	{
		memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

		if (ent->d_type == DT_DIR)
		{
			FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		}

		snprintf(lpath, PATH_MAX, "%s/%s", path, ent->d_name);

		if (virt_stat(lpath, &buf) == 0)
		{
			FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;

			FindData->dwFileAttributes |= 0x80000000;
			FindData->dwReserved0 = buf.st_mode;
			UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
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
	snprintf(path, PATH_MAX, "%s%s", base, Path);
	DIR *cur = virt_opendir(path);

	if (cur != NULL && SetFindData(cur, FindData) == true)
		return (HANDLE)cur;

	return (HANDLE)(-1);
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	DIR *cur = (DIR*)Hdl;
	return SetFindData(cur, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	DIR *cur = (DIR*)Hdl;

	if (cur != NULL)
		virt_closedir(cur);

	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int ifd, ofd, done;
	ssize_t len, total = 0;
	char rpath[PATH_MAX + 1];
	char buff[8192];
	int result = FS_FILE_OK;
	struct stat buf;

	snprintf(rpath, PATH_MAX, "%s%s", base, RemoteName);


	if (gProgressProc(gPluginNr, rpath, LocalName, 0) == 1)
		return FS_FILE_USERABORT;

	ifd = virt_open(rpath, O_RDONLY, 0);

	if (ifd == -1)
		return FS_FILE_READERROR;

	ofd = open(LocalName, O_WRONLY | O_CREAT, S_IRWXU);

	if (ofd > -1)
	{
		while ((len = virt_read(ifd, buff, sizeof(buff))) > 0)
		{
			if (write(ofd, buff, len) == -1)
				result = FS_FILE_WRITEERROR;

			total += len;

			if (buf.st_size > 0)
				done = total * 100 / buf.st_size;
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

	if (virt_stat(rpath, &buf) == 0)
		chmod(LocalName, buf.st_mode);

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
			gRequestProc(gPluginNr, RT_TargetDir, "AVFS", "Enter AVFS path:", base, PATH_MAX - 1);
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
