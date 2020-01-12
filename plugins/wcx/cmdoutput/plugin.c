#define _GNU_SOURCE
#include <glib.h>
#include <gio/gio.h>
#include <dlfcn.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

#define errmsg(msg) gStartupInfo->MessageBox((char*)msg, NULL, MB_OK | MB_ICONERROR);
#define GROUP_MAX 255

typedef struct sArcData
{
	gchar arcname[PATH_MAX + 1];
	gchar group[GROUP_MAX + 1];
	gsize current;
	gsize total;
	gchar **files;
	GKeyFile *cfg;
	tProcessDataProc gProcessDataProc;
} tArcData;

typedef tArcData* ArcData;

typedef void *HINSTANCE;

tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gStartupInfo = NULL;

static gchar *cfg_path;

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gStartupInfo == NULL)
	{
		gStartupInfo = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gStartupInfo, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gStartupInfo != NULL)
		free(gStartupInfo);

	if (cfg_path)
		g_free(cfg_path);
}

static gchar *get_file_ext(const gchar *Filename)
{
	if (g_file_test(Filename, G_FILE_TEST_IS_DIR))
		return NULL;

	gchar *basename, *result, *tmpval;

	basename = g_path_get_basename(Filename);
	result = g_strrstr(basename, ".");

	if (result)
	{
		if (g_strcmp0(result, basename) != 0)
		{
			tmpval = g_strdup_printf("%s", result + 1);
			result = g_ascii_strdown(tmpval, -1);
			g_free(tmpval);
		}
		else
			result = NULL;
	}

	g_free(basename);

	return result;
}

static gchar *get_mime_type(const gchar *Filename)
{
	GFile *gfile = g_file_new_for_path(Filename);

	if (!gfile)
		return NULL;

	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);

	if (!fileinfo)
		return NULL;

	gchar *content_type = g_strdup(g_file_info_get_content_type(fileinfo));
	g_object_unref(fileinfo);
	g_object_unref(gfile);

	return content_type;
}

static gchar *str_replace(gchar *text, gchar *str, gchar *repl, gboolean quote)
{
	gchar *result = NULL;

	if (!str || !repl || !text)
		return result;

	gchar **split = g_strsplit(text, str, -1);

	if (quote)
	{
		gchar *quoted_repl = g_shell_quote(repl);
		result = g_strjoinv(quoted_repl, split);
		g_free(quoted_repl);
	}
	else
		result = g_strjoinv(repl, split);

	g_strfreev(split);

	return result;
}

static gchar *basenamenoext(const gchar *file)
{
	gchar *basename = g_path_get_basename(file);
	gchar *result = g_strdup(basename);
	gchar *dot = g_strrstr(result, ".");

	if (dot)
	{
		int offset = dot - result;
		result[offset] = '\0';

		if (result[0] == '\0')
			result = g_strdup(basename);
	}

	g_free(basename);
	return result;
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	tArcData *handle = g_new0(tArcData, 1);

	if (handle == NULL)
	{
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	g_strlcpy(handle->arcname, ArchiveData->ArcName, PATH_MAX);
	handle->cfg = g_key_file_new();

	if (!g_key_file_load_from_file(handle->cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}
	else
	{
		gchar *mime_type = get_mime_type(ArchiveData->ArcName);
		gchar *file_ext = get_file_ext(ArchiveData->ArcName);

		if (file_ext && g_key_file_has_group(handle->cfg, file_ext))
		{
			handle->files = g_key_file_get_keys(handle->cfg, file_ext, &handle->total, NULL);
			g_strlcpy(handle->group, file_ext, GROUP_MAX);
		}
		else if (mime_type && g_key_file_has_group(handle->cfg, mime_type))
		{
			handle->files = g_key_file_get_keys(handle->cfg, mime_type, &handle->total, NULL);
			g_strlcpy(handle->group, mime_type, GROUP_MAX);
		}
		else
		{
			ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
			return E_SUCCESS;
		}

		if (mime_type)
			g_free(mime_type);

		if (file_ext)
			g_free(file_ext);

	}

	return (HANDLE)handle;

}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	memset(HeaderData, 0, sizeof(&HeaderData));
	ArcData handle = (ArcData)hArcData;

	if (handle->current > handle->total || !handle->files[handle->current])
		return E_END_ARCHIVE;

	gchar *filename = basenamenoext(handle->arcname);

	if (handle->files[handle->current][0] != '.' && handle->files[handle->current][0] != '_')
		filename = g_strdup(handle->files[handle->current]);
	else
		filename = g_strdup_printf("%s%s", filename, handle->files[handle->current]);

	g_strlcpy(HeaderData->FileName, filename, sizeof(HeaderData->FileName) - 1);
	HeaderData->UnpSize = 1024;
	handle->current++;
	g_free(filename);
	return E_SUCCESS;

}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	ArcData handle = (ArcData)hArcData;
	GError *err = NULL;
	int result = E_SUCCESS;

	if (Operation == PK_TEST)
		return E_NOT_SUPPORTED;

	if (Operation == PK_EXTRACT)
	{
		gchar *command = g_key_file_get_string(handle->cfg, handle->group, handle->files[handle->current - 1], &err);

		if (err)
		{
			errmsg((err)->message);
			result = E_EABORTED;
			g_error_free(err);
		}
		else if (command)
		{
			gchar *tmp = str_replace(command, "$FILE", handle->arcname, TRUE);
			g_free(command);
			command = str_replace(tmp, "$OUTPUT", DestName, TRUE);
			g_free(tmp);

			if (system(command) != 0)
			{
				gchar *msg = g_strdup_printf("Failed to execute command \"%s\"", command);
				errmsg(msg);
				g_free(msg);
				result = E_EABORTED;
			}

			g_free(command);
		}
	}

	return result;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData handle = (ArcData)hArcData;
	g_key_file_free(handle->cfg);
	g_strfreev(handle->files);
	g_free(handle);
	return E_SUCCESS;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_HIDE | PK_CAPS_BY_CONTENT | PK_CAPS_OPTIONS;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData handle = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !handle)
		gProcessDataProc = pProcessDataProc;
	else
		handle->gProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{

}

void DCPCALL PackSetDefaultParams(PackDefaultParamStruct* dps)
{
	gchar *ini_dirname = g_path_get_dirname(dps->DefaultIniName);
	cfg_path = g_strdup_printf("%s/wcx_cmdoutput.ini", ini_dirname);
	g_free(ini_dirname);

	if (!g_file_test(cfg_path, G_FILE_TEST_EXISTS))
	{
		GKeyFile *cfg = g_key_file_new();
		Dl_info dlinfo;
		static char default_cfg[PATH_MAX + 1];

		memset(&dlinfo, 0, sizeof(dlinfo));

		if (dladdr(default_cfg, &dlinfo) != 0)
		{
			g_strlcpy(default_cfg, dlinfo.dli_fname, PATH_MAX);
			char *pos = strrchr(default_cfg, '/');

			if (pos)
				strcpy(pos + 1, "settings_default.ini");
		}

		if (!g_key_file_load_from_file(cfg, default_cfg, G_KEY_FILE_KEEP_COMMENTS, NULL))
		{
			g_key_file_set_string(cfg, "image/png", ".jpg", "convert $FILE $OUTPUT");
		}

		g_key_file_save_to_file(cfg, cfg_path, NULL);
		g_key_file_free(cfg);
	}
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	GError *err = NULL;
	gboolean result = FALSE;
	GKeyFile *cfg = g_key_file_new();

	gchar *mime_type = get_mime_type(FileName);
	gchar *file_ext = get_file_ext(FileName);

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		gchar *msg = g_strdup_printf("%s: %s", cfg_path, (err)->message);
		errmsg(msg);
		g_free(msg);
	}
	else if ((mime_type && g_key_file_has_group(cfg, mime_type)) ||
	                (file_ext && g_key_file_has_group(cfg, file_ext)))
		result = TRUE;

	if (mime_type)
		g_free(mime_type);

	if (file_ext)
		g_free(file_ext);

	if (err)
		g_error_free(err);

	if (cfg)
		g_key_file_free(cfg);

	return result;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	gchar *quted_path = g_shell_quote(cfg_path);
	gchar *command = g_strdup_printf("xdg-open %s", quted_path);
	system(command);
	g_free(quted_path);
	g_free(command);
}
