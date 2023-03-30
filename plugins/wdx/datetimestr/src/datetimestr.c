#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include "wdxplugin.h"

static GKeyFile *gCfg = NULL;
static gchar *gCfgPath = NULL;

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	int result = ft_string;

	if (FieldIndex == 0)
		g_key_file_load_from_file(gCfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL);

	gchar *string = g_strdup_printf("FieldName%d", FieldIndex);
	gchar *field = g_key_file_get_string(gCfg, PLUGNAME, string, NULL);
	g_free(string);

	if (!field)
		return ft_nomorefields;

	if (field[0] == '\0')
	{
		string = g_strdup_printf("Preset%d", FieldIndex);
		g_strlcpy(FieldName, string, maxlen - 1);
		g_free(string);
	}
	else
		g_strlcpy(FieldName, field, maxlen - 1);

	g_free(field);
	g_strlcpy(Units, "modified|access|changed", maxlen - 1);
	string = g_strdup_printf("IsNumeric%d", FieldIndex);

	if (g_key_file_get_boolean(gCfg, PLUGNAME, string, NULL))
		result = ft_numeric_64;

	g_free(string);

	return result;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	GFile *gfile;
	GFileInfo *info;
	const char *attr;
	int result = ft_string;

	gfile = g_file_new_for_path(FileName);

	if (!gfile)
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

	info = g_file_query_info(gfile, attr, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (!info)
	{
		g_object_unref(gfile);
		return ft_fieldempty;
	}

	guint64 unixtime = g_file_info_get_attribute_uint64(info, attr);

	if (unixtime == 0)
	{
		g_object_unref(gfile);
		g_object_unref(info);
		return ft_fieldempty;
	}

	gchar *string = g_strdup_printf("Format%d", FieldIndex);
	gchar *format = g_key_file_get_string(gCfg, PLUGNAME, string, NULL);
	g_free(string);

	if (!format || format[0] == '\0')
	{
		g_free(format);
		g_object_unref(gfile);
		g_object_unref(info);
		return ft_fieldempty;
	}

	GDateTime *date = g_date_time_new_from_unix_local(unixtime);
	gchar *formated = g_date_time_format(date, format);
	g_date_time_unref(date);
	g_free(format);

	string = g_strdup_printf("IsNumeric%d", FieldIndex);

	if (g_key_file_get_boolean(gCfg, PLUGNAME, string, NULL))
	{
		*(int64_t*)FieldValue = (int64_t)g_ascii_strtoll(formated, NULL, 0);
		result = ft_numeric_64;
	}
	else
		g_strlcpy((char*)FieldValue, formated, maxlen - 1);

	g_free(string);
	g_free(formated);
	g_object_unref(gfile);
	g_object_unref(info);

	return result;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	if (!gCfg)
		gCfg = g_key_file_new();

	gchar *cfgdir = g_path_get_dirname(dps->DefaultIniName);
	gCfgPath = g_strdup_printf("%s/%s", cfgdir, "j2969719.ini");

	if (!g_key_file_load_from_file(gCfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL) || !g_key_file_has_group(gCfg, PLUGNAME))
	{
		g_key_file_set_string(gCfg, PLUGNAME, "FieldName0", "Date");
		g_key_file_set_string(gCfg, PLUGNAME, "Format0", "%a %d %b %k:%M");
		g_key_file_set_comment(gCfg, PLUGNAME, "Format0", " https://developer-old.gnome.org/glib/stable/glib-GDateTime.html#g-date-time-format", NULL);
		g_key_file_set_boolean(gCfg, PLUGNAME, "IsNumeric0", FALSE);
		g_key_file_set_comment(gCfg, PLUGNAME, "IsNumeric0", " value contains digits only", NULL);
		g_key_file_set_string(gCfg, PLUGNAME, "FieldName1", "Day");
		g_key_file_set_string(gCfg, PLUGNAME, "Format1", "%a");
		g_key_file_set_boolean(gCfg, PLUGNAME, "IsNumeric1", FALSE);
		g_key_file_set_string(gCfg, PLUGNAME, "FieldName2", "Year");
		g_key_file_set_string(gCfg, PLUGNAME, "Format2", "%Y");
		g_key_file_set_boolean(gCfg, PLUGNAME, "IsNumeric2", TRUE);
		g_key_file_set_string(gCfg, PLUGNAME, "FieldName3", "Time");
		g_key_file_set_string(gCfg, PLUGNAME, "Format3", "%T");
		g_key_file_set_boolean(gCfg, PLUGNAME, "IsNumeric3", FALSE);
		g_key_file_save_to_file(gCfg, gCfgPath, NULL);
	}
}

void DCPCALL ContentPluginUnloading(void)
{
	if (gCfgPath)
	{
		g_free(gCfgPath);
		gCfgPath = NULL;
	}

	if (gCfg)
	{
		g_key_file_free(gCfg);
		gCfg = NULL;
	}
}
