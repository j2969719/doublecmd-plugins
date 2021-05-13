#include <glib.h>
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
	{"GeneticName",		ft_string,	  "locale|default"},
	{"Exec",		ft_string,			""},
	{"TryExec",		ft_string,			""},
	{"Path",		ft_string,			""},
	{"URL",			ft_string,			""},
	{"Icon",		ft_string,	  "locale|default"},
	{"Categories",		ft_string,			""},
	{"Hidden",		ft_boolean,			""},
	{"NoDisplay",		ft_boolean,			""},
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
	{"PrefersNonDefaultGPU",ft_boolean,			""},
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
	g_strlcpy(DetectString, _detectstring, maxlen - 1);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	gboolean is_true;
	GKeyFile *keyfile;
	GError *err = NULL;
	gchar *string = NULL;
	int result = ft_fieldempty;

	keyfile = g_key_file_new();

	if (g_key_file_load_from_file(keyfile, FileName, 0, NULL))
	{
		switch (fields[FieldIndex].type)
		{
		case ft_string:
			if (fields[FieldIndex].unit[0] != '\0' && UnitIndex == 0)
			{
				const gchar * const *lang_names = g_get_language_names();

				while (*lang_names)
				{
					gchar *key = g_strdup_printf("%s[%s]", fields[FieldIndex].name, *lang_names);
					string = g_key_file_get_string(keyfile, "Desktop Entry", key, NULL);
					g_free(key);

					if (string)
						break;

					*lang_names++;
				}

				if (!string)
					string = g_key_file_get_string(keyfile, "Desktop Entry", fields[FieldIndex].name, NULL);
			}
			else
				string = g_key_file_get_string(keyfile, "Desktop Entry", fields[FieldIndex].name, NULL);

			if (string)
			{
				g_strlcpy((char*)FieldValue, string, maxlen - 1);
				result = ft_string;
			}

			g_free(string);

			break;

		case ft_boolean:
			is_true = g_key_file_get_boolean(keyfile, "Desktop Entry", fields[FieldIndex].name, &err);

			if (err)
				g_error_free(err);
			else
			{
				*(int*)FieldValue = (int)is_true;
				result = ft_boolean;
			}

			break;
		}
	}

	g_key_file_free(keyfile);
	return result;
}
