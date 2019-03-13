#include <glib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "wfxplugin.h"

#define _plugname "tmppanel_crap"
#define _inifile "tmppanel_crap.ini"
#define _dirmark "folder"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
GKeyFile *cfg;
gchar *cfgpath = "";

gchar **files;
gchar currentgroup[PATH_MAX];
gsize ifile;


gboolean UnixTimeToFileTime(unsigned long mtime, LPFILETIME ft)
{
	gint64 ll = Int32x32To64(mtime, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
	return TRUE;
}

void GetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

gboolean SaveCfgFile(void)
{
	GError *err = NULL;

	if (!g_key_file_save_to_file(cfg, cfgpath, &err))
	{
		//g_print("%s(%s): %s\n", _plugname, cfgpath, (err)->message);
		gRequestProc(gPluginNr, RT_MsgOK, _plugname, (err)->message, NULL, 0);

		if (err)
			g_error_free(err);

		return FALSE;
	}

	return TRUE;
}

gboolean SetFindData(WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
	struct stat buf;

	if ((files) && (files[ifile] != NULL))
	{
		g_strlcpy(FindData->cFileName, files[ifile], PATH_MAX);
		gchar *target = g_key_file_get_string(cfg, currentgroup, files[ifile], NULL);

		if ((target) && (g_strcmp0(target, _dirmark) == 0))
		{
			GetCurrentFileTime(&FindData->ftLastWriteTime);
			FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | 0x80000000;
			FindData->dwReserved0 = 16877;
		}
		else
		{
			if ((lstat(target, &buf) == 0))
			{
				FindData->nFileSizeLow = buf.st_size;
				FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
				FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
				FindData->dwFileAttributes |= 0x80000000;
				FindData->dwReserved0 = buf.st_mode;
				UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
				UnixTimeToFileTime(buf.st_atime, &FindData->ftLastAccessTime);
			}
			else
			{
				FindData->nFileSizeLow = 0;
				GetCurrentFileTime(&FindData->ftLastWriteTime);
			}
		}

		ifile++;
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
	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	SaveCfgFile();
	gsize count;
	ifile = 0;
	g_strlcpy(currentgroup, Path, PATH_MAX);

	if ((g_strcmp0(currentgroup, "/") != 0) && (currentgroup[(strlen(currentgroup) - 1)] == '/'))
		currentgroup[(strlen(currentgroup) - 1)] = '\0';

	files = g_key_file_get_keys(cfg, currentgroup, &count, NULL);

	if (SetFindData(FindData))
		return (HANDLE)(1985);
	else
		return (HANDLE)(-1);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	if (SetFindData(FindData))
		return TRUE;
	else
		return FALSE;
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	return 0;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);
	gchar *result = g_key_file_get_string(cfg, group, key, NULL);
	g_free(group);
	g_free(key);

	if (result)
	{
		g_strlcpy(RemoteName, result, maxlen - 1);
		g_free(result);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	gchar *group = g_path_get_dirname(Path);
	gchar *key = g_path_get_basename(Path);

	if (!g_key_file_has_key(cfg, group, key, NULL))
	{
		g_key_file_set_string(cfg, group, key, _dirmark);
		SaveCfgFile();
	}

	g_free(group);
	g_free(key);
	return TRUE;
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	gboolean result = FALSE;
	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);

	if (g_key_file_remove_key(cfg, group, key, NULL))
	{
		g_key_file_remove_group(cfg, RemoteName, NULL);
		//if (SaveCfgFile())
		result = TRUE;
	}

	g_free(group);
	g_free(key);
	return result;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	gboolean result = FALSE;
	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);

	if (g_key_file_remove_key(cfg, group, key, NULL))
	{
		//if (SaveCfgFile())
		result = TRUE;
	}

	g_free(group);
	g_free(key);
	return result;

}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);

	if (err)
		return FS_FILE_USERABORT;

	struct stat buf;

	if ((stat(LocalName, &buf) != 0) || (!S_ISREG(buf.st_mode)))
		return FS_FILE_OK;

	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);

	if ((CopyFlags == 0) && (g_key_file_has_key(cfg, group, key, NULL)))
	{
		g_free(group);
		g_free(key);
		return FS_FILE_EXISTS;
	}

	g_key_file_set_string(cfg, group, key, LocalName);
	g_free(group);
	g_free(key);
	gProgressProc(gPluginNr, RemoteName, LocalName, 50);

	//if (!SaveCfgFile())
	//	return FS_FILE_WRITEERROR;

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);
	return FS_FILE_OK;
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct* ri)
{
	int err = gProgressProc(gPluginNr, OldName, NewName, 0);

	if (err)
		return FS_FILE_USERABORT;

	int result = FS_FILE_OK;
	gchar *group = g_path_get_dirname(OldName);
	gchar *key = g_path_get_basename(OldName);
	gchar *value = g_key_file_get_string(cfg, group, key, NULL);
	gchar *newgroup = g_path_get_dirname(NewName);
	gchar *newkey = g_path_get_basename(NewName);

	if ((OverWrite == FALSE) && (g_key_file_has_key(cfg, newgroup, newkey, NULL)))
		result = FS_FILE_EXISTS;
	else
	{
		g_key_file_set_string(cfg, newgroup, newkey, value);
		gProgressProc(gPluginNr, OldName, NewName, 50);
		if (Move)
		{
			gsize count;
			g_key_file_remove_key(cfg, group, key, NULL);
			g_key_file_get_keys(cfg, group, &count, NULL);
			if (count == 0)
			{
				gchar *parent = g_path_get_dirname(group);
				gchar *mark = g_path_get_basename(group);
				g_key_file_remove_key(cfg, parent, mark, NULL);
				g_key_file_remove_group(cfg, group, NULL);
				g_free(parent);
				g_free(mark);
			}
		}
	}

	g_free(group);
	g_free(key);
	g_free(value);
	g_free(newgroup);
	g_free(newkey);
	gProgressProc(gPluginNr, OldName, NewName, 100);
	return result;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	int result = FS_EXEC_ERROR;
	gchar *group = g_path_get_dirname(RemoteName);
	gchar *key = g_path_get_basename(RemoteName);
	gchar *path = g_key_file_get_string(cfg, group, key, NULL);
	g_free(group);
	g_free(key);

	if (g_strcmp0(Verb, "open") == 0)
	{
		//not working
		//result = FS_EXEC_YOURSELF;

		gchar *command = g_strdup_printf("xdg-open \"%s\"", path);
		g_spawn_command_line_async(command, NULL);
		g_free(command);
		result = FS_FILE_OK;
	}
	else if (g_ascii_strncasecmp(Verb, "chmod", 5) == 0)
	{
		gint i = g_ascii_strtoll(Verb+6, 0, 8);
		if (chmod(path, i) == -1)
		{
			int errsv = errno;
			gRequestProc(gPluginNr, RT_MsgOK, _plugname, strerror(errsv), NULL, 0);
		}
		result = FS_FILE_OK;
	}
	else
	{
		g_spawn_command_line_async("xdg-open https://www.youtube.com/watch?v=dQw4w9WgXcQ", NULL);
	}

	g_free(path);
	return result;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, _plugname, maxlen - 1);
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	GError *err = NULL;

	cfgpath = g_strdup_printf("%s/%s", g_path_get_dirname(dps->DefaultIniName), _inifile);

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfgpath, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("%s(%s): %s\n", _plugname, cfgpath, (err)->message);

	if (err)
		g_error_free(err);
}
