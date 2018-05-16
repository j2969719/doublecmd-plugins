#include <glib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include "wdxplugin.h"

#define _detectstring "EXT=\"desktop\""

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{"Name",		ft_string,	  "locale|default"},
	{"Comment",		ft_string,	  "locale|default"},
	{"Genetic Name",	ft_string,	  "locale|default"},
	{"Exec",		ft_string,			""},
	{"TryExec",		ft_string,			""},
	{"Path",		ft_string,			""},
	{"URL",			ft_string,			""},
	{"Icon",		ft_string,	  "locale|default"},
	{"Categories",		ft_string,			""},
	{"Hidden",		ft_boolean,			""},
	{"NoDisplay",		ft_boolean,			""},
	{"Shown in menus",	ft_boolean,			""},
	{"Terminal",		ft_boolean,			""},
	{"StartupNotify",	ft_boolean,			""},
	{"DBusActivatable",	ft_boolean,			""},
	{"StartupWMClass",	ft_string,			""},
	{"Type",		ft_string,			""},
	{"OnlyShowIn",		ft_string,			""},
	{"NotShowIn",		ft_string,			""},
	{"MimeType",		ft_string,			""},
	{"Actions",		ft_string,			""},
	{"Keywords",		ft_string,	  "locale|default"},
	{"Implements",		ft_string,			""},
};

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	g_strlcpy(Units, fields[FieldIndex].unit, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	const gchar *tmp;
	gboolean vempty = FALSE;
	GDesktopAppInfo *info = g_desktop_app_info_new_from_filename(FileName);

	if (!info)
		return ft_fileerror;

	switch (FieldIndex)
	{
	case 8:
		tmp = g_desktop_app_info_get_categories(info);

		if (tmp)
			g_strlcpy((char*)FieldValue, tmp, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 9:
		if (g_desktop_app_info_get_is_hidden(info))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 10:
		if (g_desktop_app_info_get_nodisplay(info))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 11:
		if (g_desktop_app_info_get_show_in(info, NULL))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 12:
		if (!g_desktop_app_info_has_key(info, "Terminal"))
			return ft_fieldempty;

		if (g_desktop_app_info_get_boolean(info, "Terminal"))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 13:
		if (!g_desktop_app_info_has_key(info, "StartupNotify"))
			return ft_fieldempty;

		if (g_desktop_app_info_get_boolean(info, "StartupNotify"))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 14:
		if (!g_desktop_app_info_has_key(info, "DBusActivatable"))
			return ft_fieldempty;

		if (g_desktop_app_info_get_boolean(info, "DBusActivatable"))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	default:
		if ((fields[FieldIndex].unit == "") || (UnitIndex > 0))
			tmp = g_desktop_app_info_get_string(info, fields[FieldIndex].name);
		else
			tmp = g_desktop_app_info_get_locale_string(info, fields[FieldIndex].name);

		if (tmp)
			g_strlcpy((char*)FieldValue, tmp, maxlen - 1);
		else
			vempty = TRUE;

		break;
	}

	if (vempty)
		return ft_fieldempty;
	else
		return fields[FieldIndex].type;
}

