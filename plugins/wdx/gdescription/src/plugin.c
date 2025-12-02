#include <glib.h>
#include <gio/gio.h>
#include "wdxplugin.h"

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex == 0)
	{
		g_strlcpy(FieldName, "description", maxlen - 1);
		g_strlcpy(Units, "default|follow symlink", maxlen - 1);
		return ft_string;
	}
	else if (FieldIndex == 1)
	{
		g_strlcpy(FieldName, "content type", maxlen - 1);
		Units[0] == '\0';
		return ft_string;
	}
	else if (FieldIndex == 2)
	{
		g_strlcpy(FieldName, "type", maxlen - 1);
		g_strlcpy(Units, "audio|video|image", maxlen - 1);
		return ft_multiplechoice;
	}
	else
		return ft_nomorefields;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, "EXT=\"*\"", maxlen - 1);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	gchar *string;
	gchar **split;
	int result = ft_fieldempty;

	GFile *gfile = g_file_new_for_path(FileName);

	if (!gfile)
		return ft_fileerror;

	int query_flags = G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS;

	if (FieldIndex == 0 && UnitIndex == 1)
		query_flags = 0;

	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, query_flags, NULL, NULL);

	if (!fileinfo)
	{
		g_object_unref(gfile);
		return ft_fileerror;
	}

	const gchar *content_type = g_file_info_get_content_type(fileinfo);

	if (content_type)
	{
		switch (FieldIndex)
		{
		case 0:
			string = g_content_type_get_description(content_type);

			if (string)
			{
				g_strlcpy((char*)FieldValue, string, maxlen - 1);
				g_free(string);
				result = ft_string;
			}

			break;

		case 1:
			g_strlcpy((char*)FieldValue, content_type, maxlen - 1);
			result = ft_string;
			break;

		case 2:
			split = g_strsplit(content_type, "/", -1);

			if (split)
			{
				g_strlcpy((char*)FieldValue, split[0], maxlen - 1);
				g_strfreev(split);
				result = ft_multiplechoice;
			}

			break;
		}
	}

	g_object_unref(fileinfo);
	g_object_unref(gfile);
	return result;
}
