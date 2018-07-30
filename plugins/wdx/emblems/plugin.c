#include <glib.h>
#include <gio/gio.h>
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
	GFile *gfile = g_file_new_for_path (FileName);
	if (!gfile)
		return ft_fileerror;

	GFileInfo *fileinfo = g_file_query_info(gfile, "metadata::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
	if (!fileinfo)
		return ft_fileerror;
	gchar *result = g_file_info_get_attribute_as_string(fileinfo, "metadata::emblems");
	if (result && g_strcmp0(result, "[]") != 0)
	{
		g_strlcpy((char*)FieldValue, result, maxlen-1);
		return ft_string;
	}
	return ft_fieldempty;
}

