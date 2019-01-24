#define _GNU_SOURCE

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "wdxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define imagetypes "jpeg|png|gif|svg|bmp|ico|xpm"

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{N_("width"),		ft_numeric_32,			""},
	{N_("height"),		ft_numeric_32,			""},
	{N_("size"),		ft_string,			""},
	{N_("type"),		ft_multiplechoice,	imagetypes},
	{N_("description"),	ft_string,			""},
};

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, gettext(fields[FieldIndex].name), maxlen-1);
	g_strlcpy(Units, fields[FieldIndex].unit, maxlen-1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int width;
	int height;
	gchar *tmp;

	if (!g_file_test(FileName, G_FILE_TEST_IS_REGULAR))
		return ft_fileerror;

	GdkPixbufFormat *fileinfo = gdk_pixbuf_get_file_info(FileName, &width, &height);

	if (!fileinfo)
		return ft_fileerror;

	switch (FieldIndex)
	{
	case 0:
		*(int*)FieldValue = width;
		break;

	case 1:
		*(int*)FieldValue = height;
		break;

	case 2:
		tmp = g_strdup_printf("%dx%d", width, height);

		if (tmp)
			g_strlcpy((char*)FieldValue, tmp, maxlen-1);
		else
			return ft_fieldempty;

		break;

	case 3:
		tmp = gdk_pixbuf_format_get_name(fileinfo);

		if (tmp)
			g_strlcpy((char*)FieldValue, tmp, maxlen-1);
		else
			return ft_fieldempty;

		break;

	case 4:
		tmp = gdk_pixbuf_format_get_description(fileinfo);

		if (tmp)
			g_strlcpy((char*)FieldValue, tmp, maxlen-1);
		else
			return ft_fieldempty;

		break;

	default:
		return ft_nosuchfield;
	}

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
