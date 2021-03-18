#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <physfs.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))

typedef struct sVFSDirData
{
	char** files;
	char** cur;
	char* path;
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;

static char gLFMPath[PATH_MAX];

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
	char *fname;
	PHYSFS_Stat stat;
	bool found = false;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (dirdata->cur != NULL && *dirdata->cur != NULL)
	{
		strlcpy(FindData->cFileName, *dirdata->cur, MAX_PATH - 1);

		if (dirdata->path[1] != '\0')
			asprintf(&fname, "%s/%s", dirdata->path, *dirdata->cur);
		else
			asprintf(&fname, "/%s", *dirdata->cur);

		if (PHYSFS_stat(fname, &stat) != 0)
		{
			FindData->nFileSizeHigh = (stat.filesize & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = stat.filesize & 0x00000000FFFFFFFF;
			UnixTimeToFileTime(stat.createtime, &FindData->ftCreationTime);
			UnixTimeToFileTime(stat.accesstime, &FindData->ftLastAccessTime);

			if (stat.modtime > 0)
				UnixTimeToFileTime(stat.modtime, &FindData->ftLastWriteTime);
			else
				UnixTimeToFileTime(time(0), &FindData->ftLastWriteTime);

			switch (stat.filetype)
			{
			case PHYSFS_FILETYPE_DIRECTORY:
				FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
				break;

			case PHYSFS_FILETYPE_SYMLINK:
				FindData->dwFileAttributes = FILE_ATTRIBUTE_REPARSE_POINT;
				break;

			case PHYSFS_FILETYPE_REGULAR:
				FindData->dwFileAttributes = 0x00000080;
				break;

			case PHYSFS_FILETYPE_OTHER:
				break;
			}

			if (stat.readonly != 0)
				FindData->dwFileAttributes |= 0x00000001;
		}
		else
			UnixTimeToFileTime(time(0), &FindData->ftLastWriteTime);

		free(fname);
		++dirdata->cur;

		return true;
	}

	return found;
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_ENABLE, 0, 0);
		gDialogApi->SendDlgMsg(pDlg, "btnDelete", DM_ENABLE, 0, 0);
		gDialogApi->SendDlgMsg(pDlg, "btnUp", DM_ENABLE, 0, 0);
		gDialogApi->SendDlgMsg(pDlg, "btnDown", DM_ENABLE, 0, 0);

		const PHYSFS_ArchiveInfo **i;

		for (i = PHYSFS_supportedArchiveTypes(); *i != NULL; i++)
		{
			char *line;
			asprintf(&line, "%s (%s)%s", (*i)->description, (*i)->extension,
			         (*i)->supportsSymlinks > 0 ? ", symlinks supported" : "");
			gDialogApi->SendDlgMsg(pDlg, "mArcSupported", DM_LISTADD, (intptr_t)line, 0);
			free(line);
		}

		char **path;
		char **search_path = PHYSFS_getSearchPath();

		for (path = search_path; *path != NULL; path++)
			gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTADD, (intptr_t)*path, 0);

		PHYSFS_freeList(search_path);

		int count = (int)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETCOUNT, 0, 0);

		if (count > 0)
		{
			gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_ENABLE, 1, 0);
			gDialogApi->SendDlgMsg(pDlg, "btnDelete", DM_ENABLE, 1, 0);
			gDialogApi->SendDlgMsg(pDlg, "btnUp", DM_ENABLE, 1, 0);
			gDialogApi->SendDlgMsg(pDlg, "btnDown", DM_ENABLE, 1, 0);
		}

		gDialogApi->SendDlgMsg(pDlg, "chkSymlinks", DM_SETCHECK, (intptr_t)PHYSFS_symbolicLinksPermitted(), 0);

		break;

	case DN_CLICK:

		if (strcmp(DlgItemName, "btnAdd") == 0)
		{
			char *newpath = (char*)gDialogApi->SendDlgMsg(pDlg, "fneAddFile", DM_GETTEXT, 0, 0);

			if (newpath != NULL && access(newpath, F_OK) == 0)
			{
				gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_ENABLE, 1, 0);
				gDialogApi->SendDlgMsg(pDlg, "btnDelete", DM_ENABLE, 1, 0);
				gDialogApi->SendDlgMsg(pDlg, "btnUp", DM_ENABLE, 1, 0);
				gDialogApi->SendDlgMsg(pDlg, "btnDown", DM_ENABLE, 1, 0);
				gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTADD, (intptr_t)newpath, 0);
				gDialogApi->SendDlgMsg(pDlg, "fneAddFile", DM_SETTEXT, 0, 0);
			}
		}
		else if (strcmp(DlgItemName, "btnDelete") == 0)
		{
			int i = (int)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETITEMINDEX, 0, 0);

			if (i > -1)
				gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTDELETE, i, 0);

			int count = (int)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETCOUNT, 0, 0);

			if (count < 1)
			{
				gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_ENABLE, 0, 0);
				gDialogApi->SendDlgMsg(pDlg, "btnDelete", DM_ENABLE, 0, 0);
				gDialogApi->SendDlgMsg(pDlg, "btnUp", DM_ENABLE, 0, 0);
				gDialogApi->SendDlgMsg(pDlg, "btnDown", DM_ENABLE, 0, 0);
			}
		}
		else if (strcmp(DlgItemName, "btnUp") == 0)
		{
			int i = (int)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETITEMINDEX, 0, 0);

			if (i > -1)
			{
				i--;

				if (i > -1)
				{
					char *path = (char*)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETITEM, i + 1, 0);
					gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTDELETE, i + 1, 0);
					gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTINSERT, i, (intptr_t)path);
					gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTSETITEMINDEX, i, 0);
				}
			}
		}
		else if (strcmp(DlgItemName, "btnDown") == 0)
		{
			int i = (int)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETITEMINDEX, 0, 0);

			if (i > -1)
			{
				i++;
				int count = (int)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETCOUNT, 0, 0);

				if (i < count)
				{
					char *path = (char*)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETITEM, i - 1, 0);
					gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTDELETE, i - 1, 0);
					gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTINSERT, i, (intptr_t)path);
					gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTSETITEMINDEX, i, 0);
				}
			}
		}
		else if (strcmp(DlgItemName, "btnOK") == 0)
		{
			char **path;
			char **search_path = PHYSFS_getSearchPath();

			for (path = search_path; *path != NULL; path++)
				PHYSFS_unmount(*path);

			PHYSFS_freeList(search_path);

			char *newpath;
			int count = (int)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETCOUNT, 0, 0);

			for (int i = 0; i < count; i++)
			{
				newpath = (char*)gDialogApi->SendDlgMsg(pDlg, "lbMounts", DM_LISTGETITEM, i, 0);
				PHYSFS_mount(newpath, NULL, i + 1);
			}

			PHYSFS_permitSymbolicLinks((int)gDialogApi->SendDlgMsg(pDlg, "chkSymlinks", DM_GETCHECK, 0, 0));

			gDialogApi->SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_OK, 0);
		}
		else if (strcmp(DlgItemName, "btnCancel") == 0)
		{
			gDialogApi->SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_CANCEL, 0);
		}

		break;
	}

	return 0;
}

static void ShowCFGDlg(void)
{

	if (gDialogApi != NULL)
	{
		if (access(gLFMPath, F_OK) != 0)
		{
			if (gRequestProc && gRequestProc(gPluginNr, RT_MsgYesNo, NULL, "Unmount all archives?", NULL, 0))
			{
				char **path;
				char **search_path = PHYSFS_getSearchPath();

				for (path = search_path; *path != NULL; path++)
					PHYSFS_unmount(*path);

				PHYSFS_freeList(search_path);
			}


		}
		else
			gDialogApi->DialogBoxLFMFile(gLFMPath, DlgProc);
	}
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

	dirdata->files = PHYSFS_enumerateFiles(Path);
	dirdata->cur = dirdata->files;
	dirdata->path = strdup(Path);

	if (SetFindData(dirdata, FindData) == true)
		return (HANDLE)dirdata;

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

	if (dirdata->files != NULL)
		PHYSFS_freeList(dirdata->files);

	if (dirdata->path != NULL)
		free(dirdata->path);

	if (dirdata != NULL)
		free(dirdata);

	return 0;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strcmp(Verb, "open") == 0)
	{
		return FS_EXEC_YOURSELF;
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		ShowCFGDlg();
		return FS_EXEC_OK;
	}
	else if (strncmp(Verb, "quote", 5) == 0)
	{
		if (strcmp(Verb + 6, "ver") == 0)
		{
			char *msg;
			PHYSFS_Version compiled, linked;
			PHYSFS_VERSION(&compiled);
			PHYSFS_getLinkedVersion(&linked);
			asprintf(&msg, "Compiled against PhysFS v%d.%d.%d, linked against PhysFS v%d.%d.%d",
			         compiled.major, compiled.minor, compiled.patch, linked.major, linked.minor, linked.patch);
			gRequestProc(gPluginNr, RT_MsgOK, NULL, msg, NULL, 0);
			free(msg);
		}

		return FS_EXEC_OK;
	}

	return FS_EXEC_ERROR;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int fd, done;
	char buff[8192];
	PHYSFS_Stat stat;
	PHYSFS_sint64 file_size;
	ssize_t len, total = 0;
	int result = FS_FILE_OK;

	if (CopyFlags == 0 && access(LocalName, F_OK) == 0)
		return FS_FILE_EXISTS;

	if (PHYSFS_stat(RemoteName, &stat) != 0)
	{
		if (stat.filetype != PHYSFS_FILETYPE_REGULAR)
			return FS_FILE_NOTSUPPORTED;

		file_size = stat.filesize;
	}
	else return FS_FILE_READERROR;

	PHYSFS_file* inFile = PHYSFS_openRead(RemoteName);

	fd = open(LocalName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (fd != -1)
	{
		while ((len = PHYSFS_readBytes(inFile, buff, sizeof(buff))) > 0)
		{
			if (write(fd, buff, len) == -1)
			{
				result = FS_FILE_WRITEERROR;
				break;
			}

			total += len;

			if (file_size > 0)
				done = total * 100 / (ssize_t)file_size;
			else
				done = 0;

			if (done > 100)
				done = 100;

			if (gProgressProc(gPluginNr, RemoteName, LocalName, done) == 1)
			{
				result = FS_FILE_USERABORT;
				break;
			}
		}

		close(fd);
	}

	PHYSFS_close(inFile);

	return result;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int i = 0;
	char **path;

	char **search_path = PHYSFS_getSearchPath();

	for (path = search_path; *path != NULL; path++)
		i++;

	PHYSFS_freeList(search_path);

	if (PHYSFS_mount(LocalName, NULL, i + 1) == 0)
		return FS_FILE_NOTSUPPORTED;
	else
		return FS_FILE_OK;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	strlcpy(DefRootName, "PhysFS", maxlen - 1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gDialogApi == NULL)
	{
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));
		strlcpy(gLFMPath, gDialogApi->PluginDir, PATH_MAX);
		strncat(gLFMPath, "dialog.lfm", PATH_MAX - 11);
		PHYSFS_init(NULL);
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gDialogApi != NULL)
	{
		if (PHYSFS_isInit() != 0)
			PHYSFS_deinit();

		free(gDialogApi);
	}

	gDialogApi = NULL;
}
