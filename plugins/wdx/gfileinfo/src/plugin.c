#include <glib.h>
#include <stdio.h>
#include <gio/gio.h>
#include "string.h"
#include "wdxplugin.h"

typedef struct sfield
{
	char *attr;
	int type;
} tfield;

#define fieldcount (sizeof(gFields)/sizeof(tfield))
#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))

tfield gFields[] =
{
	{G_FILE_ATTRIBUTE_STANDARD_NAME,		    ft_string},
	{G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,	    ft_string},
	{G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME,		    ft_string},
	{G_FILE_ATTRIBUTE_STANDARD_COPY_NAME,		    ft_string},
	{G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION,		    ft_string},
	{G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,	    ft_string},
	{G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE,	    ft_string},
	{G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,	    ft_string},
	{G_FILE_ATTRIBUTE_STANDARD_TARGET_URI,		    ft_string},
	{G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,		   ft_boolean},
	{G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP,		   ft_boolean},
	{G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK,		   ft_boolean},
	{G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL,		   ft_boolean},
	{G_FILE_ATTRIBUTE_STANDARD_IS_VOLATILE,		   ft_boolean},
	{G_FILE_ATTRIBUTE_STANDARD_SIZE,		ft_numeric_64},
	{G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE,	ft_numeric_64},
	{G_FILE_ATTRIBUTE_ETAG_VALUE,			    ft_string},
	{G_FILE_ATTRIBUTE_ID_FILE,			    ft_string},
	{G_FILE_ATTRIBUTE_ID_FILESYSTEM,		    ft_string},
	{G_FILE_ATTRIBUTE_ACCESS_CAN_READ,		   ft_boolean},
	{G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,		   ft_boolean},
	{G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE,		   ft_boolean},
	{G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE,		   ft_boolean},
	{G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,		   ft_boolean},
	{G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME,		   ft_boolean},
	{G_FILE_ATTRIBUTE_TIME_MODIFIED,		  ft_datetime},
	{G_FILE_ATTRIBUTE_TIME_ACCESS,			  ft_datetime},
	{G_FILE_ATTRIBUTE_TIME_CHANGED,			  ft_datetime},
	{G_FILE_ATTRIBUTE_TIME_CREATED,			  ft_datetime},
	{G_FILE_ATTRIBUTE_UNIX_DEVICE,			ft_numeric_32},
	{G_FILE_ATTRIBUTE_UNIX_INODE,			ft_numeric_64},
	{G_FILE_ATTRIBUTE_UNIX_MODE,			ft_numeric_32},
	{G_FILE_ATTRIBUTE_UNIX_NLINK,			ft_numeric_32},
	{G_FILE_ATTRIBUTE_UNIX_UID,			ft_numeric_32},
	{G_FILE_ATTRIBUTE_UNIX_GID,			ft_numeric_32},
	{G_FILE_ATTRIBUTE_UNIX_RDEV,			ft_numeric_32},
	{G_FILE_ATTRIBUTE_UNIX_BLOCK_SIZE,		ft_numeric_32},
	{G_FILE_ATTRIBUTE_UNIX_BLOCKS,			ft_numeric_64},
	{G_FILE_ATTRIBUTE_UNIX_IS_MOUNTPOINT,		   ft_boolean},
	{G_FILE_ATTRIBUTE_OWNER_USER,			    ft_string},
	{G_FILE_ATTRIBUTE_OWNER_USER_REAL,		    ft_string},
	{G_FILE_ATTRIBUTE_OWNER_GROUP,			    ft_string},
	{G_FILE_ATTRIBUTE_THUMBNAIL_PATH,		    ft_string},
	{G_FILE_ATTRIBUTE_THUMBNAILING_FAILED,		   ft_boolean},
	{G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID,		   ft_boolean},
	{G_FILE_ATTRIBUTE_SELINUX_CONTEXT,		    ft_string},
};

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, gFields[FieldIndex].attr, maxlen - 1);
	Units[0] = '\0';
	return gFields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, "EXT=\"*\"", maxlen - 1);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	gint64 ll;
	gboolean is_empty = FALSE;
	const gchar *str_result = NULL;

	GFile *gfile = g_file_new_for_path(FileName);

	if (!gfile)
		return ft_fileerror;

	GFileInfo *fileinfo = g_file_query_info(gfile, gFields[FieldIndex].attr, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

	if (!fileinfo)
	{
		g_object_unref(gfile);
		return ft_fieldempty;
	}

	if (g_file_info_get_attribute_type(fileinfo, gFields[FieldIndex].attr) == G_FILE_ATTRIBUTE_TYPE_INVALID)
	{
		g_object_unref(fileinfo);
		g_object_unref(gfile);
		return ft_fieldempty;
	}

	switch (gFields[FieldIndex].type)
	{
	case ft_string:
		if (g_file_info_get_attribute_type(fileinfo, gFields[FieldIndex].attr) == G_FILE_ATTRIBUTE_TYPE_BYTE_STRING)
			str_result = g_file_info_get_attribute_byte_string(fileinfo, gFields[FieldIndex].attr);
		else
			str_result = g_file_info_get_attribute_string(fileinfo, gFields[FieldIndex].attr);

		if (!str_result)
			is_empty = TRUE;
		else
			g_strlcpy((char*)FieldValue, str_result, maxlen - 1);

		break;

	case ft_numeric_32:
		if (g_file_info_get_attribute_type(fileinfo, gFields[FieldIndex].attr) == G_FILE_ATTRIBUTE_TYPE_UINT32)
		{
			if (g_strcmp0(gFields[FieldIndex].attr, "unix::mode") == 0)
			{
				char mode[7];
				snprintf(mode, sizeof(mode), "%o", g_file_info_get_attribute_uint32(fileinfo, gFields[FieldIndex].attr));
				*(int*)FieldValue = atoi(mode);
			}
			else
				*(gint32*)FieldValue = (gint32)g_file_info_get_attribute_uint32(fileinfo, gFields[FieldIndex].attr);
		}
		else
			*(gint32*)FieldValue = g_file_info_get_attribute_int32(fileinfo, gFields[FieldIndex].attr);

		break;

	case ft_numeric_64:
		if (g_file_info_get_attribute_type(fileinfo, gFields[FieldIndex].attr) == G_FILE_ATTRIBUTE_TYPE_UINT64)
			*(gint64*)FieldValue = (gint64)g_file_info_get_attribute_uint64(fileinfo, gFields[FieldIndex].attr);
		else
			*(gint64*)FieldValue = g_file_info_get_attribute_int64(fileinfo, gFields[FieldIndex].attr);

		break;

	case ft_boolean:
		*(int*)FieldValue = g_file_info_get_attribute_boolean(fileinfo, gFields[FieldIndex].attr);
		break;

	case ft_datetime:
		if (g_file_info_get_attribute_type(fileinfo, gFields[FieldIndex].attr) == G_FILE_ATTRIBUTE_TYPE_UINT64)
			ll = (gint64)g_file_info_get_attribute_uint64(fileinfo, gFields[FieldIndex].attr);
		else
			ll = g_file_info_get_attribute_int64(fileinfo, gFields[FieldIndex].attr);

		ll = Int32x32To64(ll, 10000000) + 116444736000000000;
		((LPFILETIME)FieldValue)->dwLowDateTime = (DWORD)ll;
		((LPFILETIME)FieldValue)->dwHighDateTime = ll >> 32;
		break;
	}

	g_object_unref(fileinfo);
	g_object_unref(gfile);

	if (is_empty)
		return ft_fieldempty;
	else
		return gFields[FieldIndex].type;
}
