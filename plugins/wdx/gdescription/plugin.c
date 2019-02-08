#include <glib.h>
#include <gio/gio.h>
#include "wdxplugin.h"

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	g_strlcpy(Units, "", maxlen-1);

	if (FieldIndex == 0)
	{
		g_strlcpy(FieldName, "description", maxlen-1);
		return ft_string;
	}
	else if (FieldIndex == 1)
	{
		g_strlcpy(FieldName, "content type", maxlen-1);
		return ft_string;
	}
	else if (FieldIndex == 2)
	{
		g_strlcpy(FieldName, "type", maxlen-1);
		g_strlcpy(Units, "audio|video|image", maxlen-1);
		return ft_multiplechoice;
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
	if (FieldIndex >= 0 && FieldIndex < 3)
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

		if (FieldIndex == 1)
		{
			g_strlcpy((char*)FieldValue, content_type, maxlen-1);
			return ft_string;
		}
		else if (FieldIndex == 2)
		{
			gchar **type = g_regex_split_simple("/", content_type, 0, 0);

			if (!type || !type[0])
				return ft_fieldempty;

			g_strlcpy((char*)FieldValue, type[0], maxlen-1);
			g_strfreev(type);
			return ft_multiplechoice;
		}

		gchar *result = g_content_type_get_description(content_type);

		if (!result)
			return ft_fieldempty;

		g_strlcpy((char*)FieldValue, result, maxlen-1);
		return ft_string;
	}

	return ft_fieldempty;
}
