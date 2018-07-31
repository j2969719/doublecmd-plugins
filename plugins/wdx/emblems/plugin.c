#define _GNU_SOURCE
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <limits.h>
#include <dlfcn.h>
#include "wdxplugin.h"

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex != 0)
		return ft_nomorefields;
	g_strlcpy(FieldName, "emblems", maxlen-1);
	return ft_string;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	Dl_info dlinfo;
	static char cfg_path[PATH_MAX];
	const char* cfg_file = "settings.ini";
	GKeyFile *cfg;
	gchar *result = "";
	gchar *emblem;
	gchar *frmtstr;

	GFile *gfile = g_file_new_for_path (FileName);
	if (!gfile)
		return ft_fileerror;

	GFileInfo *fileinfo = g_file_query_info(gfile, "metadata::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
	if (!fileinfo)
		return ft_fileerror;
	gchar **emblems = g_file_info_get_attribute_stringv(fileinfo, "metadata::emblems");

	memset(&dlinfo, 0, sizeof(dlinfo));
	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);

		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}

	cfg = g_key_file_new();
	g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL);
	frmtstr = g_key_file_get_string(cfg, "Settings", "Format", NULL);
	if (!frmtstr)
		frmtstr = "%s %s";

	if (emblems)
	{
		gsize i = 0;
		while (emblems[i] != NULL)
		{
			emblem = g_key_file_get_string(cfg, "Emblems", emblems[i], NULL);
			if (!emblem)
				emblem = g_strdup(emblems[i]);

			if (g_strcmp0(result, "") != 0)
			{
				gchar *tmp = g_strdup(result);
				result = g_strdup_printf(frmtstr, tmp, emblem);
			}
			else
				result = g_strdup(emblem);
			i++;
		}
		g_key_file_free(cfg);
		g_strlcpy((char*)FieldValue, result, maxlen-1);
		return ft_string;
	}
	g_key_file_free(cfg);
	return ft_fieldempty;
}

