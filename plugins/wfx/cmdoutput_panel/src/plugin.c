#define _GNU_SOURCE
#include <glib.h>
#include <dlfcn.h>
#include <string.h>
#include <errno.h>
#include "wfxplugin.h"
#include "extension.h"

typedef struct sVFSDirData
{
	gchar **files;
	gsize i;
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
GKeyFile *gCfg;
static gchar *gCfgPath = NULL;

gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (dirdata->files[dirdata->i] != NULL)
	{
		g_strlcpy(FindData->cFileName, dirdata->files[dirdata->i], MAX_PATH);
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
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

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);
	dirdata->i = 0;
	dirdata->files = g_key_file_get_keys(gCfg, PLUGNAME, NULL, NULL);

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

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "some kind of crap", maxlen - 1);
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	GError *err = NULL;

	gCfg = g_key_file_new();

	gchar *cfgdir = g_path_get_dirname(dps->DefaultIniName);

	if (cfgdir)
	{
		gCfgPath = g_strdup_printf("%s/j2969719.ini", cfgdir);
		g_free(cfgdir);
	}

	if (!g_key_file_load_from_file(gCfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, &err) || !g_key_file_has_group(gCfg, PLUGNAME))
	{
		if (err)
			g_print("%s (%s): %s\n", PLUGNAME, gCfgPath, (err)->message);

		g_key_file_set_string(gCfg, PLUGNAME, "IP.txt", "curl ipinfo.io/ip");
		g_key_file_set_string(gCfg, PLUGNAME, "uptime.txt", "uptime -p");
		g_key_file_set_string(gCfg, PLUGNAME, "temp1", "sh -c 'sensors | grep \"temp1\" -m1 | cut -c16-22'");
		g_key_file_set_string(gCfg, PLUGNAME, "temp2", "sh -c 'sensors | grep \"temp2\" -m1 | cut -c16-22'");
		g_key_file_set_string(gCfg, PLUGNAME, "temp3", "sh -c 'sensors | grep \"temp3\" -m1 | cut -c16-22'");

		g_key_file_save_to_file(gCfg, gCfgPath, NULL);
	}

	if (err)
		g_error_free(err);
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex > 0)
		return ft_nomorefields;

	g_strlcpy(FieldName, "Value", maxlen - 1);
	Units[0] = '\0';

	return ft_string;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	gchar *output = NULL;
	gchar *command = g_key_file_get_string(gCfg, PLUGNAME, FileName + 1, NULL);

	if (!command)
		return ft_fieldempty;

	if (!g_spawn_command_line_sync(command, &output, NULL, NULL, NULL))
	{
		g_free(command);
		return ft_fieldempty;
	}

	g_free(command);

	if (!output)
		return ft_fieldempty;

	size_t len = strlen(output);

	if (len > 0 && output[len - 1] == '\n')
		output[len - 1] = '\0';

	g_strlcpy((char*)FieldValue, output, maxlen - 1);
	g_free(output);

	return ft_string;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	g_strlcpy(ViewContents, "[Plugin(FS).Value{}]", maxlen - 1);
	g_strlcpy(ViewHeaders, "Value", maxlen - 1);
	g_strlcpy(ViewWidths, "100,0,100", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	return;
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gCfg)
		g_key_file_free(gCfg);

	if (gCfgPath)
	{
		g_free(gCfgPath);
		gCfgPath = NULL;
	}
}
