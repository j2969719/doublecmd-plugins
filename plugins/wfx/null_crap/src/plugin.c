#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "wfxplugin.h"
#include "extension.h"

#define MessageBox gExtensions->MessageBox
#define ROOTNAME "NULL"
#define BUFSIZE 8192

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;


static void LogDialog(void)
{
	if (gLogProc && MessageBox("Do you want to see the log?", NULL, MB_YESNO | MB_ICONQUESTION) ==  ID_YES)
		gLogProc(gPluginNr, MSGTYPE_CONNECT, "CONNECT /dev/null");
}

static void LogError(char *Info, char *FileName, int err)
{
	char *msg;
	asprintf(&msg, "%s %s: %s", Info, FileName, strerror(err));
	gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, msg);
	free(msg);
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;

	LogDialog();

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	return (HANDLE)(-1);
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	return false;
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	return 0;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int fd, done;
	ssize_t len, total = 0;
	char buff[BUFSIZE];
	struct stat buf;
	int result = FS_FILE_OK;

	if (gProgressProc(gPluginNr, LocalName, ROOTNAME, 0) == 1)
		return FS_FILE_USERABORT;

	if (stat(LocalName, &buf) != 0)
	{
		int errsv = errno;
		LogError("stat", LocalName, errsv);
		return FS_FILE_READERROR;
	}

	fd = open(LocalName, O_RDONLY);

	if (fd == -1)
	{
		int errsv = errno;
		LogError("open", LocalName, errsv);
		return FS_FILE_READERROR;
	}

	while ((len = read(fd, buff, sizeof(buff))) > 0)
	{
		total += len;

		if (buf.st_size > 0)
			done = total * 100 / buf.st_size;
		else
			done = 0;

		if (done > 100)
			done = 100;

		if (gProgressProc(gPluginNr, LocalName, ROOTNAME, done) == 1)
		{
			result = FS_FILE_USERABORT;
			break;
		}
	}

	if (len == -1)
	{
		int errsv = errno;
		LogError("read", LocalName, errsv);
		result = FS_FILE_READERROR;
	}

	if (close(fd) == -1)
	{
		int errsv = errno;
		LogError("close", LocalName, errsv);
		result = FS_FILE_READERROR;
	}

	return result;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	return true;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strcmp(Verb, "properties") == 0)
	{
		LogDialog();
		return FS_EXEC_OK;
	}

	return FS_EXEC_ERROR;
}

BOOL DCPCALL FsDisconnect(char* DisconnectRoot)
{
	gLogProc(gPluginNr, MSGTYPE_DISCONNECT, "DISCONNECT /dev/null");
	return true;
}

int DCPCALL FsGetBackgroundFlags(void)
{
	return BG_UPLOAD;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	snprintf(DefRootName, maxlen - 1, ROOTNAME);
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
