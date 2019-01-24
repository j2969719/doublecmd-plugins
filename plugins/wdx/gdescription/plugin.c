#define _GNU_SOURCE

#include <glib.h>
#include <gio/gio.h>
#include "wdxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex == 0)
	{
		g_strlcpy(FieldName, _("description"), maxlen-1);
		return ft_string;
	}
	else
		return ft_nomorefields;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, "EXT=\"*\"", maxlen-1);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (FieldIndex == 0)
	{
		GFile *gfile = g_file_new_for_path(FileName);

		if (!gfile)
			return ft_fileerror;

		GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

		if (!fileinfo)
			return ft_fileerror;

		const gchar *content_type = g_file_info_get_content_type(fileinfo);

		if (!content_type)
			return ft_fieldempty;

		gchar *result = g_content_type_get_description(content_type);

		if (!result)
			return ft_fieldempty;

		g_strlcpy((char*)FieldValue, result, maxlen-1);
		return ft_string;
	}

	return ft_fieldempty;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	const gchar* dir_f = "%s/langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(dir_f, &dlinfo) != 0)
	{
		setlocale(LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, g_strdup_printf(dir_f,
		                g_path_get_dirname(dlinfo.dli_fname)));
		textdomain(GETTEXT_PACKAGE);
	}
}
