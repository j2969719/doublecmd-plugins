#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <linux/limits.h>
#include <string.h>
#include "wdxplugin.h"

#include <dlfcn.h>

#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

unsigned int calccount(const char *name, unsigned char type)
{
	DIR *dir;
	unsigned int count = 0;
	struct dirent *ent;

	if ((dir = opendir(name)) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
			{
				if (ent->d_type == type)
					count ++;

				if (ent->d_type == DT_DIR)
				{
					char path[PATH_MAX];
					sprintf(path, "%s/%s", name, ent->d_name);
					count += calccount(path, type);
				}
			}
		}

		closedir(dir);
	}

	return count;
}

int emptychk(const char *name, bool subf)
{
	DIR *dir;
	struct dirent *ent;

	if ((dir = opendir(name)) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
			{
				if ((subf == true) && (ent->d_type == DT_DIR))
				{
					char path[PATH_MAX];
					sprintf(path, "%s/%s", name, ent->d_name);

					if (emptychk(path, subf) == 0)
					{
						closedir(dir);
						return 0;
					}
				}
				else
				{
					closedir(dir);
					return 0;
				}
			}
		}

		closedir(dir);
	}

	return 1;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	switch (FieldIndex)
	{
	case 0:
		strncpy(FieldName, _("is empty"), maxlen - 1);
		return ft_boolean;

	case 1:
		strncpy(FieldName, _("contains empty dirs only"), maxlen - 1);
		return ft_boolean;

	case 2:
		strncpy(FieldName, _("contains (dir)"), maxlen - 1);
		return ft_numeric_32;

	case 3:
		strncpy(FieldName, _("contains (regular file)"), maxlen - 1);
		return ft_numeric_32;

	case 4:
		strncpy(FieldName, _("contains (link)"), maxlen - 1);
		return ft_numeric_32;

	case 5:
		strncpy(FieldName, _("contains (fifo)"), maxlen - 1);
		return ft_numeric_32;

	case 6:
		strncpy(FieldName, _("contains (socket)"), maxlen - 1);
		return ft_numeric_32;

	case 7:
		strncpy(FieldName, _("contains (block device)"), maxlen - 1);
		return ft_numeric_32;

	case 8:
		strncpy(FieldName, _("contains (character device)"), maxlen - 1);
		return ft_numeric_32;

	case 9:
		strncpy(FieldName, _("contains (unknown)"), maxlen - 1);
		return ft_numeric_32;

	default:
		return ft_nomorefields;
	}
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	DIR* dir;

	if (strncmp(FileName + strlen(FileName) - 3, "/..", 4) == 0)
		return ft_fileerror;

	if ((dir = opendir(FileName)) == NULL)
		return ft_fileerror;

	closedir(dir);

	switch (FieldIndex)
	{
	case 0:
		*(int*)FieldValue = emptychk(FileName, false);
		return ft_boolean;

	case 1:
		*(int*)FieldValue = emptychk(FileName, true);
		return ft_boolean;

	case 2:
		*(unsigned int*)FieldValue = calccount(FileName, DT_DIR);
		break;

	case 3:
		*(unsigned int*)FieldValue = calccount(FileName, DT_REG);
		break;

	case 4:
		*(unsigned int*)FieldValue = calccount(FileName, DT_LNK);
		break;

	case 5:
		*(unsigned int*)FieldValue = calccount(FileName, DT_FIFO);
		break;

	case 6:
		*(unsigned int*)FieldValue = calccount(FileName, DT_SOCK);
		break;

	case 7:
		*(unsigned int*)FieldValue = calccount(FileName, DT_BLK);
		break;

	case 8:
		*(unsigned int*)FieldValue = calccount(FileName, DT_CHR);
		break;

	case 9:
		*(unsigned int*)FieldValue = calccount(FileName, DT_UNKNOWN);
		break;

	default:
		return ft_fieldempty;
	}

	return ft_numeric_32;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	static char plg_path[PATH_MAX];
	const char* loc_dir = "langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(plg_path, &dlinfo) != 0)
	{
		strncpy(plg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plg_path, '/');

		if (pos)
			strcpy(pos + 1, loc_dir);

		setlocale (LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, plg_path);
		textdomain(GETTEXT_PACKAGE);
	}
}
