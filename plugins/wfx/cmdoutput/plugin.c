#define _GNU_SOURCE
#include <glib.h>
#include <dlfcn.h>
#include <string.h>
#include "extension.h"
#include "wfxplugin.h"

#define _plugname "CMD Output"
#define _inifile "settings.ini"
#define _inicmd "CommandString"
#define _filesize 0

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
GKeyFile *cfg;
gchar **files;
gsize count, i;

void GetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;
	Dl_info dlinfo;
	GError *err = NULL;
	gchar *cfgpath = "";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfgpath, &dlinfo) != 0)
	{
		cfgpath = g_path_get_dirname(dlinfo.dli_fname);
		cfgpath = g_strdup_printf("%s/%s", cfgpath, _inifile);
	}

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfgpath, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("%s(%s): %s", _plugname, cfgpath, (err)->message);
	else
		files = g_key_file_get_groups(cfg, &count);

	if (err)
		g_error_free(err);

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
	i = 0;

	if (files[i] != NULL)
	{
		g_strlcpy(FindData->cFileName, files[i], PATH_MAX);
		GetCurrentFileTime(&FindData->ftLastWriteTime);

		FindData->nFileSizeLow = _filesize;
		i++;
	}

	return (HANDLE)(1985);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (files[i] != NULL)
	{
		g_strlcpy(FindData->cFileName, files[i], PATH_MAX);
		GetCurrentFileTime(&FindData->ftLastWriteTime);

		FindData->nFileSizeLow = _filesize;

		i++;
		return TRUE;
	}
	else
		return FALSE;
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);
	GError *gerr = NULL;

	if (err)
		return FS_FILE_USERABORT;

	if ((CopyFlags == 0) && (g_file_test(LocalName, G_FILE_TEST_EXISTS)))
		return FS_FILE_EXISTS;

	gchar *cmdsrt = g_key_file_get_string(cfg, RemoteName + 1, _inicmd, &gerr);

	if (gerr)
	{
		g_print("%s(%s): %s", _plugname, _inicmd, (gerr)->message);
		g_error_free(gerr);
	}

	gchar *command = g_strdup_printf(cmdsrt, LocalName, LocalName);

	if (system(command) == -1)
		return FS_FILE_WRITEERROR;

	return FS_FILE_OK;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (g_strcmp0(Verb, "open") == 0)
	{
		return FS_EXEC_YOURSELF;
	}

	return FS_EXEC_OK;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, _plugname, maxlen - 1);
}
