#define _GNU_SOURCE
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <limits.h>
#include <dlfcn.h>
#include "wdxplugin.h"

GKeyFile *cfg;
static char cfg_path[PATH_MAX];
const char* cfg_file = "settings.ini";

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex == 0)
	{
		g_strlcpy(FieldName, "emblems", maxlen - 1);
		return ft_string;
	}
	else if (FieldIndex == 1)
	{
		gchar *emblems = "";
		GKeyFile *cfg = g_key_file_new();
		g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL);
		gchar **keys = g_key_file_get_keys(cfg, "Emblems", NULL, NULL);
		gsize i = 0;

		while (keys[i] != NULL)
		{
			gchar *emblem = g_key_file_get_string(cfg, "Emblems", keys[i], NULL);

			if (emblem)
			{
				if (g_strcmp0(emblems, "") != 0)
				{
					gchar *tmp = g_strdup(emblems);
					emblems = g_strdup_printf("%s|%s", tmp, emblem);
					g_free(tmp);
				}
				else
					emblems = g_strdup(emblem);
			}

			g_free(emblem);
			i++;
		}

		g_strlcpy(FieldName, "contain", maxlen - 1);
		g_strlcpy(Units, emblems, maxlen - 1);
		g_strfreev(keys);
		return ft_boolean;
	}

	return ft_nomorefields;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	gchar *result = "";
	gchar *emblem;
	gchar *frmtstr;

	GFile *gfile = g_file_new_for_path(FileName);

	if (!gfile)
		return ft_fileerror;

	GFileInfo *fileinfo = g_file_query_info(gfile, "metadata::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (!fileinfo)
		return ft_fileerror;

	gchar **emblems = g_file_info_get_attribute_stringv(fileinfo, "metadata::emblems");

	g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL);
	frmtstr = g_key_file_get_string(cfg, "Settings", "Format", NULL);

	if (!frmtstr)
		frmtstr = "%s %s";


	gsize i = 0;

	if (emblems && emblems[i] != NULL)
	{
		while (emblems[i] != NULL)
		{
			emblem = g_key_file_get_string(cfg, "Emblems", emblems[i], NULL);

			if (!emblem)
				emblem = g_strdup(emblems[i]);

			if (g_strcmp0(result, "") != 0)
			{
				gchar *tmp = g_strdup(result);
				result = g_strdup_printf(frmtstr, tmp, emblem);
				g_free(tmp);
			}
			else
				result = g_strdup(emblem);

			i++;
		}

		g_strfreev(emblems);

		if (FieldIndex == 1)
		{
			gchar **keys = g_key_file_get_keys(cfg, "Emblems", NULL, NULL);
			emblem = g_key_file_get_string(cfg, "Emblems", keys[UnitIndex], NULL);
			g_strfreev(keys);

			if (g_strrstr(result, emblem) != NULL)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;

			g_free(emblem);
			return ft_boolean;
		}
		else if (FieldIndex == 0)
		{
			g_free(emblem);
			g_strlcpy((char*)FieldValue, result, maxlen - 1);
			return ft_string;
		}
	}

	return ft_fieldempty;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);

		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}

	cfg = g_key_file_new();
}

void DCPCALL ContentPluginUnloading(void)
{
	g_key_file_free(cfg);
}
