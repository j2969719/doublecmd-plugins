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

FIELD fields[]={
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
	g_strlcpy(FieldName, fields[FieldIndex].name, maxlen-1);
	g_strlcpy(Units, "last modified|last accessed|last changed", maxlen-1);
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
	if (strncmp(FileName+strlen(FileName)-3, "/..", 4) == 0) 
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
		g_strlcpy((char*)FieldValue, tmp, maxlen-1);
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
	gchar *tmp;
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
		tmp = g_key_file_get_string(cfg, "Preset1", "Name", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
			fields[0].name = tmp;
		tmp = g_key_file_get_string(cfg, "Preset1", "Format", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
		{
			fields[0].format = tmp;
			if (g_strcmp0(fields[0].name, "placeholder") == 0)
				fields[0].name = fields[0].format;
		}
		bval = g_key_file_get_boolean(cfg, "Preset1", "Numeric", NULL);
		if (bval == TRUE)
			fields[0].type = ft_numeric_64;

		tmp = g_key_file_get_string(cfg, "Preset2", "Name", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
			fields[1].name = tmp;
		tmp = g_key_file_get_string(cfg, "Preset2", "Format", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
		{
			fields[1].format = tmp;
			if (g_strcmp0(fields[1].name, "placeholder") == 0)
				fields[1].name = fields[1].format;
		}
		bval = g_key_file_get_boolean(cfg, "Preset2", "Numeric", NULL);
		if (bval == TRUE)
			fields[1].type = ft_numeric_64;

		tmp = g_key_file_get_string(cfg, "Preset3", "Name", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
			fields[2].name = tmp;
		tmp = g_key_file_get_string(cfg, "Preset3", "Format", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
		{
			fields[2].format = tmp;
			if (g_strcmp0(fields[2].name, "placeholder") == 0)
				fields[2].name = fields[2].format;
		}
		bval = g_key_file_get_boolean(cfg, "Preset3", "Numeric", NULL);
		if (bval == TRUE)
			fields[2].type = ft_numeric_64;

		tmp = g_key_file_get_string(cfg, "Preset4", "Name", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
			fields[3].name = tmp;
		tmp = g_key_file_get_string(cfg, "Preset4", "Format", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
		{
			fields[3].format = tmp;
			if (g_strcmp0(fields[3].name, "placeholder") == 0)
				fields[3].name = fields[3].format;
		}
		bval = g_key_file_get_boolean(cfg, "Preset4", "Numeric", NULL);
		if (bval == TRUE)
			fields[3].type = ft_numeric_64;

		tmp = g_key_file_get_string(cfg, "Preset5", "Name", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
			fields[4].name = tmp;
		tmp = g_key_file_get_string(cfg, "Preset5", "Format", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
		{
			fields[4].format = tmp;
			if (g_strcmp0(fields[4].name, "placeholder") == 0)
				fields[4].name = fields[4].format;
		}
		bval = g_key_file_get_boolean(cfg, "Preset5", "Numeric", NULL);
		if (bval == TRUE)
			fields[4].type = ft_numeric_64;

		tmp = g_key_file_get_string(cfg, "Preset6", "Name", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
			fields[5].name = tmp;
		tmp = g_key_file_get_string(cfg, "Preset6", "Format", NULL);
		if ((tmp) && (g_strcmp0(tmp, "") != 0))
		{
			fields[5].format = tmp;
			if (g_strcmp0(fields[5].name, "placeholder") == 0)
				fields[5].name = fields[5].format;
		}
		bval = g_key_file_get_boolean(cfg, "Preset6", "Numeric", NULL);
		if (bval == TRUE)
			fields[5].type = ft_numeric_64;
	}
	g_key_file_free(cfg);
	if (err)
		g_error_free(err);
}
