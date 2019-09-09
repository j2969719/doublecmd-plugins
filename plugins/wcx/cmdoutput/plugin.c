#include <glib.h>
#include <gio/gio.h>
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
tExtensionStartupInfo* gStartupInfo;

static gchar *cfg_path;

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	gStartupInfo = malloc(sizeof(tExtensionStartupInfo));
	memcpy(gStartupInfo, StartupInfo, sizeof(tExtensionStartupInfo));
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	free(gStartupInfo);
}

gchar *get_file_ext(const gchar *Filename)
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

const gchar *get_mime_type(const gchar *Filename)
{
	GFile *gfile = g_file_new_for_path(Filename);

	if (!gfile)
		return NULL;

	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);

	if (!fileinfo)
		return NULL;

	const gchar *content_type = g_strdup(g_file_info_get_content_type(fileinfo));
	g_object_unref(fileinfo);
	g_object_unref(gfile);

	return content_type;
}

gchar *str_replace(gchar *text, gchar *str, gchar *repl)
{
	gchar **split = g_strsplit(text, str, -1);
	gchar *result = g_strjoinv(repl, split);
	g_strfreev(split);
	return result;
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	tArcData *handle = g_new0(tArcData, 1);
	g_strlcpy(handle->arcname, ArchiveData->ArcName, PATH_MAX);
	handle->cfg = g_key_file_new();

	if (!g_key_file_load_from_file(handle->cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}
	else
	{
		const gchar *mime_type = get_mime_type(ArchiveData->ArcName);
		const gchar *file_ext = get_file_ext(ArchiveData->ArcName);

		if (g_key_file_has_group(handle->cfg, file_ext))
		{
			handle->files = g_key_file_get_keys(handle->cfg, file_ext, &handle->total, NULL);
			g_strlcpy(handle->group, file_ext, GROUP_MAX);
		}
		else if (g_key_file_has_group(handle->cfg, mime_type))
		{
			handle->files = g_key_file_get_keys(handle->cfg, mime_type, &handle->total, NULL);
			g_strlcpy(handle->group, mime_type, GROUP_MAX);
		}
		else
		{
			ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
			return E_SUCCESS;
		}
	}

	return (HANDLE)handle;

}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	memset(HeaderData, 0, sizeof(HeaderData));
	ArcData handle = (ArcData)hArcData;

	if (handle->current > handle->total || !handle->files[handle->current])
		return E_END_ARCHIVE;

	g_strlcpy(HeaderData->FileName, handle->files[handle->current], sizeof(HeaderData->FileName) - 1);
	handle->current++;
	return E_SUCCESS;

}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	ArcData handle = (ArcData)hArcData;
	GError *err = NULL;
	int result = E_SUCCESS;

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
			command = str_replace(command, "$FILE", g_shell_quote(handle->arcname));
			command = str_replace(command, "$OUTPUT", g_shell_quote(DestName));
			g_print("command = %s\n", command);

			if (system(command) != 0)
				result = E_EWRITE;

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
	return PK_CAPS_HIDE | PK_CAPS_BY_CONTENT;
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
	cfg_path = g_strdup_printf("%s/wcx_cmdoutput.ini", g_path_get_dirname(dps->DefaultIniName));

	if (!g_file_test(cfg_path, G_FILE_TEST_EXISTS))
	{
		GKeyFile *cfg = g_key_file_new();
		g_key_file_set_string(cfg, "image/png", "output.jpg", "convert $FILE $OUTPUT");
		g_key_file_set_comment(cfg, "image/png", "output.jpg", "filename.ext=command $FILE $OUTPUT", NULL);
		g_key_file_set_string(cfg, "md", "output.html", "hoedown --all-span --all-block --all-flags --hard-wrap $FILE > $OUTPUT");
		g_key_file_save_to_file(cfg, cfg_path, NULL);
		g_key_file_free(cfg);
	}
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	GError *err = NULL;
	gboolean result = FALSE;
	GKeyFile *cfg = g_key_file_new();

	const gchar *mime_type = get_mime_type(FileName);
	const gchar *file_ext = get_file_ext(FileName);

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		errmsg((err)->message);
	}
	else if (g_key_file_has_group(cfg, mime_type) || g_key_file_has_group(cfg, file_ext))
	{
		result = TRUE;
	}

	if (err)
		g_error_free(err);

	if (cfg)
		g_key_file_free(cfg);

	return result;

}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	gchar *command = g_strdup_printf("xdg-open \"%s\"", cfg_path);
	system(command);
	g_free(command);
}
