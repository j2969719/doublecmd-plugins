#define _GNU_SOURCE
#include <glib.h>
#include <dlfcn.h>
#include <string.h>
#include <errno.h>
#include "wfxplugin.h"

typedef struct sVFSDirData
{
	gboolean groups;
	gchar **files;
	gsize i;
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
GKeyFile *gCfg;

void SetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (dirdata->files[dirdata->i] != NULL)
	{
		g_strlcpy(FindData->cFileName, dirdata->files[dirdata->i], PATH_MAX);
		SetCurrentFileTime(&FindData->ftCreationTime);
		SetCurrentFileTime(&FindData->ftLastAccessTime);
		SetCurrentFileTime(&FindData->ftLastWriteTime);

		if (dirdata->groups == TRUE)
		{
			FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | 0x80000000;
			FindData->dwReserved0 = 16877;
		}
		else
		{
			FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
			FindData->nFileSizeLow = 1024;
			FindData->dwReserved0 = 420;
		}

		dirdata->i++;
		return TRUE;
	}

	return FALSE;
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
	const gchar *inifile = "settings.ini";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfgpath, &dlinfo) != 0)
	{
		cfgpath = g_path_get_dirname(dlinfo.dli_fname);
		cfgpath = g_strdup_printf("%s/%s", cfgpath, inifile);
	}

	gCfg = g_key_file_new();

	if (!g_key_file_load_from_file(gCfg, cfgpath, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		gchar *msg = g_strdup_printf("%s: %s", cfgpath, (err)->message);
		gRequestProc(gPluginNr, RT_MsgOK, NULL, msg, NULL, 0);
		g_free(msg);
	}

	if (cfgpath)
		g_free(cfgpath);

	if (err)
		g_error_free(err);

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	dirdata->i = 0;

	if (Path[1] == '\0')
	{
		dirdata->files = g_key_file_get_groups(gCfg, NULL);
		dirdata->groups = TRUE;
	}
	else
	{
		dirdata->files = g_key_file_get_keys(gCfg, Path + 1, NULL, NULL);
		dirdata->groups = FALSE;
	}

	if (dirdata->files != NULL && SetFindData(dirdata, FindData))
		return (HANDLE)dirdata;

	if (dirdata->files != NULL)
		g_strfreev(dirdata->files);

	g_free(dirdata);

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
		g_strfreev(dirdata->files);

	g_free(dirdata);

	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int result = FS_FILE_OK;
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);
	GError *gerr = NULL;

	if (err)
		return FS_FILE_USERABORT;

	if ((CopyFlags == 0) && (g_file_test(LocalName, G_FILE_TEST_EXISTS)))
		return FS_FILE_EXISTS;

	gchar **target = g_strsplit(RemoteName + 1, "/", 2);

	gchar *cmdsrt = g_key_file_get_string(gCfg, target[0], target[1], &gerr);

	if (gerr)
	{
		gRequestProc(gPluginNr, RT_MsgOK, NULL, (gerr)->message, NULL, 0);
		g_error_free(gerr);
	}
	else
	{
		gchar *command = g_strdup_printf(cmdsrt, LocalName, LocalName);

		if (system(command) == -1)
			result = FS_FILE_WRITEERROR;

		g_free(command);
	}

	if (target)
		g_strfreev(target);

	if (cmdsrt)
		g_free(cmdsrt);

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	return result;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strncmp(Verb, "open", 5) == 0)
	{
		return FS_EXEC_YOURSELF;
	}
	else
		gRequestProc(gPluginNr, RT_MsgOK, NULL, strerror(EOPNOTSUPP), NULL, 0);

	return FS_EXEC_ERROR;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "CMDOutput", maxlen - 1);
}
