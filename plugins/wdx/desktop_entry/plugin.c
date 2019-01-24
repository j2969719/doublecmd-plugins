#define _GNU_SOURCE

#include <glib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include "wdxplugin.h"


#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

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
	{N_("Name"),		ft_string,	N_("locale|default")},
	{N_("Comment"),		ft_string,	N_("locale|default")},
	{N_("Genetic Name"),	ft_string,	N_("locale|default")},
	{N_("Exec"),		ft_string,			  ""},
	{N_("TryExec"),		ft_string,			  ""},
	{N_("Path"),		ft_string,			  ""},
	{N_("URL"),		ft_string,			  ""},
	{N_("Icon"),		ft_string,	N_("locale|default")},
	{N_("Categories"),	ft_string,			  ""},
	{N_("Hidden"),		ft_boolean,			  ""},
	{N_("NoDisplay"),	ft_boolean,			  ""},
	{N_("Shown in menus"),	ft_boolean,			  ""},
	{N_("Terminal"),	ft_boolean,			  ""},
	{N_("StartupNotify"),	ft_boolean,			  ""},
	{N_("DBusActivatable"),	ft_boolean,			  ""},
	{N_("StartupWMClass"),	ft_string,			  ""},
	{N_("Type"),		ft_string,			  ""},
	{N_("OnlyShowIn"),	ft_string,			  ""},
	{N_("NotShowIn"),	ft_string,			  ""},
	{N_("MimeType"),	ft_string,			  ""},
	{N_("Actions"),		ft_string,			  ""},
	{N_("Keywords"),	ft_string,	N_("locale|default")},
	{N_("Implements"),	ft_string,			  ""},
};

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, gettext(fields[FieldIndex].name), maxlen - 1);

	if (fields[FieldIndex].unit != "")
		g_strlcpy(Units, gettext(fields[FieldIndex].unit), maxlen - 1);
	else
		g_strlcpy(Units, "", maxlen - 1);

	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen-1);
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

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(fields, &dlinfo) != 0)
	{
		setlocale (LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, g_strdup_printf("%s/langs", g_path_get_dirname(dlinfo.dli_fname)));
		textdomain(GETTEXT_PACKAGE);
	}
}
