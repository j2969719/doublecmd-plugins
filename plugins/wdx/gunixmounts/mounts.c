#include <glib.h>
#include <gio/gunixmounts.h>
#include "wdxplugin.h"

#define _detectstring "EXT=\"*\""

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{"mountpoint",	ft_boolean,	""},
	{"name",	ft_string,	""},
	{"fs type",	ft_string,	""},
	{"device",	ft_string,	""},
	{"readonly",	ft_boolean,	""},
	{"internal",	ft_boolean,	""},
	{"can eject",	ft_boolean,	""},
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
	if (!g_file_test(FileName, G_FILE_TEST_IS_DIR))
		return ft_fileerror;

	GUnixMountEntry *test = g_unix_mount_at(FileName, NULL);

	if (!test)
		return ft_fileerror;

	const gchar *tmp;
	gboolean vempty = FALSE;

	switch (FieldIndex)
	{
	case 0:
		*(int*)FieldValue = 1;
		break;

	case 1:
		tmp = g_unix_mount_guess_name(test);

		if (tmp)
			g_strlcpy((char*)FieldValue, tmp, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 2:
		tmp = g_unix_mount_get_fs_type(test);

		if (tmp)
			g_strlcpy((char*)FieldValue, tmp, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 3:
		tmp = g_unix_mount_get_device_path(test);

		if (tmp)
			g_strlcpy((char*)FieldValue, tmp, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 4:
		if (g_unix_mount_is_readonly(test))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 5:
		if (g_unix_mount_is_system_internal(test))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 6:
		if (g_unix_mount_guess_can_eject(test))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	default:
		vempty = TRUE;
	}

	g_unix_mount_free(test);

	if (vempty)
		return ft_fieldempty;
	else
		return fields[FieldIndex].type;
}
