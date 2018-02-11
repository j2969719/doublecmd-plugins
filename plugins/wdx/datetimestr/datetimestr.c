#define _GNU_SOURCE
#include <glib.h>
#include <gio/gio.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wdxplugin.h"

typedef struct _field
{
	char *name;
	int type;
	char *format;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{"placeholder",	ft_string,	""},
	{"placeholder",	ft_string,	""},
	{"placeholder",	ft_string,	""},
	{"placeholder",	ft_string,	""},
	{"placeholder",	ft_string,	""},
	{"placeholder",	ft_string,	""},
};

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	g_strlcpy(Units, "last modified|last accessed|last changed", maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	GFile *ifile;
	GFileInfo *info;
	GDateTime *date;
	const char *attr;
	guint64 val;
	gchar *tmp;

	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_fieldempty;

	if (strncmp(FileName + strlen(FileName) - 3, "/..", 4) == 0)
		return ft_fileerror;

	if (!g_file_test(FileName, G_FILE_TEST_EXISTS))
		return ft_fileerror;

	switch (UnitIndex)
	{
	case 0:
		attr = G_FILE_ATTRIBUTE_TIME_MODIFIED;
		break;

	case 1:
		attr = G_FILE_ATTRIBUTE_TIME_ACCESS;
		break;

	case 2:
		attr = G_FILE_ATTRIBUTE_TIME_CHANGED;
		break;
	}

	ifile = g_file_new_for_path(FileName);
	info = g_file_query_info(ifile, attr, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if ((!info) || (g_strcmp0(fields[FieldIndex].format, "") == 0))
		return ft_fieldempty;

	val = g_file_info_get_attribute_uint64(info, attr);
	date = g_date_time_new_from_unix_local(val);
	tmp = g_date_time_format(date, fields[FieldIndex].format);

	if (fields[FieldIndex].type == ft_numeric_64)
	{
		val = g_ascii_strtoll(tmp, NULL, 0);
		*(guint64*)FieldValue = val;
	}
	else
		g_strlcpy((char*)FieldValue, tmp, maxlen - 1);

	g_object_unref(ifile);
	g_object_unref(info);
	g_date_time_unref(date);
	g_free(tmp);
	return fields[FieldIndex].type;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	GKeyFile *cfg;
	GError *err = NULL;
	static char cfg_path[PATH_MAX];
	const char* cfg_file = "settings.ini";
	const gchar *prsetstr;
	gchar *tmp;
	gint i;
	gboolean bval;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("datetime.wdx (%s): %s\n", cfg_path, (err)->message);
	else
	{
		for (i = 0; i < 6; i++)
		{
			prsetstr = g_strdup_printf("Preset%d", i + 1);
			tmp = g_key_file_get_string(cfg, prsetstr, "Name", NULL);

			if ((tmp) && (g_strcmp0(tmp, "") != 0))
				fields[i].name = tmp;

			tmp = g_key_file_get_string(cfg, prsetstr, "Format", NULL);

			if ((tmp) && (g_strcmp0(tmp, "") != 0))
			{
				fields[i].format = tmp;

				if (g_strcmp0(fields[i].name, "placeholder") == 0)
					fields[i].name = fields[i].format;
			}

			bval = g_key_file_get_boolean(cfg, prsetstr, "Numeric", NULL);

			if (bval == TRUE)
				fields[i].type = ft_numeric_64;
		}
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);
}
