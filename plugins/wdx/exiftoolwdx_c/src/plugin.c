#define _GNU_SOURCE
#include <glib.h>
#include <time.h>
#include <dlfcn.h>
#include <locale.h>
#include "string.h"
#include "wdxplugin.h"

typedef struct sfield
{
	char *name;
	int type;
	char *units;
	char *value;
} tfield;

#define TMPL_COMMAND "exiftool %s"
#define TMPL_DATE "%Y:%m:%d %H:%M:%S"
#define TMPL_TRUE "Yes"
#define TMPL_FALSE "No"
#define TSV_FILE PLUGNAME ".tsv"

locale_t gNumericC;
tfield *gFields = NULL;
guint gFieldCount = 0;
char gLastFile[PATH_MAX] = "";

gboolean toFileTime(char *string, LPFILETIME FileTime)
{
	struct tm t = {0};

	if (!strptime(string, TMPL_DATE, &t))
		return FALSE;

	time_t unixtime = mktime(&t);

	if (unixtime == -1)
		return FALSE;

	unixtime = (unixtime * 10000000) + 0x019DB1DED53E8000;
	FileTime->dwLowDateTime = (DWORD)unixtime;
	FileTime->dwHighDateTime = unixtime >> 32;
	return TRUE;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (!gFields || FieldIndex < 0 || FieldIndex >= gFieldCount)
		return ft_nomorefields;

	if (gFields[FieldIndex].name == NULL || gFields[FieldIndex].name[0] == '\0')
		return ft_nomorefields;
	else
		g_strlcpy(FieldName, gFields[FieldIndex].name, maxlen - 1);

	if (gFields[FieldIndex].units != NULL && strchr(gFields[FieldIndex].units, '|') != NULL)
		g_strlcpy(Units, gFields[FieldIndex].units, maxlen - 1);
	else
		Units[0] = '\0';

	return gFields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	double num;
	char *pos = NULL;
	char *end = NULL;

	if (strcmp(gLastFile, FileName) != 0)
	{
		for (int i = 0; i < gFieldCount; i++)
		{
			g_free(gFields[i].value);
			gFields[i].value = NULL;
		}

		gchar *stdout = NULL;
		gchar *quoted = g_shell_quote(FileName);
		gchar *command = g_strdup_printf(TMPL_COMMAND, quoted);
		g_free(quoted);

		if (!g_spawn_command_line_sync(command, &stdout, NULL, NULL, NULL) || !stdout)
		{
			g_free(command);
			return ft_fileerror;
		}

		g_free(command);
		gchar **split = g_strsplit(stdout, "\n", -1);

		for (gchar **p = split; *p != NULL; p++)
		{
			for (int i = 0; i < gFieldCount; i++)
			{
				if (gFields[i].value == NULL && strncmp(*p, gFields[i].name, strlen(gFields[i].name)) == 0)
				{
					char *value = strstr(*p, ": ");

					if (value)
						gFields[i].value = g_strdup(value + 2);

					break;
				}
			}
		}

		g_strfreev(split);
		g_free(stdout);
		g_strlcpy(gLastFile, FileName, PATH_MAX);
	}

	if (!gFields[FieldIndex].value || gFields[FieldIndex].value[0] == '\0')
		return ft_fieldempty;

	switch (gFields[FieldIndex].type)
	{
	case ft_string:
	case ft_multiplechoice:
		g_strlcpy((char*)FieldValue, gFields[FieldIndex].value, maxlen - 1);
		break;
	case ft_numeric_32:
		*(int*)FieldValue = atoi(gFields[FieldIndex].value);
		break;
	case ft_numeric_64:
		*(int64_t*)FieldValue = (int64_t)strtod_l(gFields[FieldIndex].value, &end, gNumericC);
		break;
	case ft_numeric_floating:
		*(double*)FieldValue = strtod_l(gFields[FieldIndex].value, &end, gNumericC);
		break;
	case ft_boolean:
		if (strcmp(gFields[FieldIndex].value, TMPL_TRUE) == 0)
			*(int*)FieldValue = 1;
		else if (strcmp(gFields[FieldIndex].value, TMPL_FALSE) == 0)
			*(int*)FieldValue = 0;
		else
			return ft_fieldempty;
		break;
	case ft_time:
	{
		char time_str[12];
		ttimeformat time_val;
		strlcpy(time_str, gFields[FieldIndex].value, sizeof(time_str));
		pos = strrchr(time_str, ':');

		if (pos)
		{
			time_val.wSecond = atoi(pos + 1);
			*pos = '\0';
			pos = strrchr(time_str, ':');

			if (pos)
			{
				time_val.wMinute = atoi(pos + 1);
				*pos = '\0';
				time_val.wHour = atoi(time_str);
				memcpy(FieldValue, (const void*)&time_val, sizeof(time_val));
				break;
			}
		}

		return ft_fieldempty;
	}
	case ft_datetime:
		if (!toFileTime(gFields[FieldIndex].value, (FILETIME*)FieldValue))
			return ft_fieldempty;

		break;
	default:
		return ft_fieldempty;
	}

	return gFields[FieldIndex].type;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	gchar *contents = NULL;
	static gchar tsv_path[PATH_MAX];

	g_strlcpy(tsv_path, dps->DefaultIniName, PATH_MAX);
	char *pos = strrchr(tsv_path, '/');

	if (pos)
		strcpy(pos + 1, TSV_FILE);

	if (!g_file_get_contents(tsv_path, &contents, NULL, NULL) || !contents)
	{
		Dl_info dlinfo;
		memset(&dlinfo, 0, sizeof(dlinfo));

		if (dladdr(tsv_path, &dlinfo) != 0)
		{
			g_strlcpy(tsv_path, dlinfo.dli_fname, PATH_MAX);
			pos = strrchr(tsv_path, '/');

			if (pos)
				strcpy(pos + 1, TSV_FILE);

			g_file_get_contents(tsv_path, &contents, NULL, NULL);
		}
	}

	if (!contents || contents[0] == '\0' || contents[0] == '\n')
		g_free(contents);
	else
	{
		gchar **split = g_strsplit(contents, "\n", -1);
		guint len = g_strv_length(split);

		if (len > 1)
		{
			gFieldCount = 0;
			gFields = malloc(len * sizeof(tfield));
			for (guint i = 1; i < len; i++)
			{
				gchar **field = g_strsplit(split[i], "\t", -1);
				guint items = g_strv_length(field);

				if (items == 3 && field[0] && field[0][0] != '\0')
				{
					gFields[gFieldCount].value = NULL;
					gFields[gFieldCount].name = g_strdup(field[0]);
					gFields[gFieldCount].type = ft_string;
					if (field[1])
					{
						if (strcmp(field[1], "ft_numeric_32") == 0)
							gFields[gFieldCount].type = ft_numeric_32;
						else if (strcmp(field[1], "ft_numeric_64") == 0)
							gFields[gFieldCount].type = ft_numeric_64;
						else if (strcmp(field[1], "ft_numeric_floating") == 0)
							gFields[gFieldCount].type = ft_numeric_floating;
						else if (strcmp(field[1], "ft_boolean") == 0)
							gFields[gFieldCount].type = ft_boolean;
						else if (strcmp(field[1], "ft_multiplechoice") == 0)
							gFields[gFieldCount].type = ft_multiplechoice;
						else if (strcmp(field[1], "ft_time") == 0)
							gFields[gFieldCount].type = ft_time;
						else if (strcmp(field[1], "ft_datetime") == 0)
							gFields[gFieldCount].type = ft_datetime;
					}

					if (field[2])
						gFields[gFieldCount].units = g_strdup(field[2]);

					gFieldCount++;
				}

				g_strfreev(field);
			}

			g_print("%s: %d field(s) from \"%s\" loaded.\n", PLUGNAME, gFieldCount, tsv_path);
		}

		g_strfreev(split);
		g_free(contents);
	}

	if (!gFields)
		g_print("%s: failed to load fields from \"%s\"!\n", PLUGNAME, TSV_FILE);

	gNumericC = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
}

void DCPCALL ContentPluginUnloading(void)
{
	if (gFields)
	{
		for (int i = 0; i < gFieldCount; i++)
		{
			g_free(gFields[i].name);
			g_free(gFields[i].units);
			g_free(gFields[i].value);
			gFields[i].value = NULL;
		}

		g_free(gFields);
	}

	freelocale(gNumericC);
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen - 1);
	return 0;
}
