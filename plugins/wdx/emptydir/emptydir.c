#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "wdxplugin.h"

static int64_t calccount(const char *name, unsigned char type)
{
	DIR *dir;
	int64_t count = 0;
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
					snprintf(path, PATH_MAX, "%s/%s", name, ent->d_name);
					count += calccount(path, type);
				}
			}
		}

		closedir(dir);
	}

	return count;
}

static int64_t calcnoacess(const char *name, bool chk)
{
	DIR *dir;
	int64_t count = 0;
	struct dirent *ent;

	if ((dir = opendir(name)) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
			{
				char path[PATH_MAX];
				snprintf(path, PATH_MAX, "%s/%s", name, ent->d_name);
				if (ent->d_type == DT_DIR && access(path, R_OK | X_OK) != 0)
					count ++;

				if (ent->d_type == DT_DIR)
					count += calcnoacess(path, chk);

				if (chk && count > 0)
					break;
			}
		}

		closedir(dir);
	}

	return count;
}

static int emptychk(const char *name, bool subf)
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
	strncpy(Units, "default|skip symlinks", maxlen - 1);

	switch (FieldIndex)
	{
	case 0:
		strncpy(FieldName, "is empty", maxlen - 1);
		return ft_boolean;

	case 1:
		strncpy(FieldName, "contains empty dirs only", maxlen - 1);
		return ft_boolean;

	case 2:
		strncpy(FieldName, "contains (dir)", maxlen - 1);
		return ft_numeric_64;

	case 3:
		strncpy(FieldName, "contains (regular file)", maxlen - 1);
		return ft_numeric_64;

	case 4:
		strncpy(FieldName, "contains (link)", maxlen - 1);
		return ft_numeric_64;

	case 5:
		strncpy(FieldName, "contains (fifo)", maxlen - 1);
		return ft_numeric_64;

	case 6:
		strncpy(FieldName, "contains (socket)", maxlen - 1);
		return ft_numeric_64;

	case 7:
		strncpy(FieldName, "contains (block device)", maxlen - 1);
		return ft_numeric_64;

	case 8:
		strncpy(FieldName, "contains (character device)", maxlen - 1);
		return ft_numeric_64;

	case 9:
		strncpy(FieldName, "contains (unknown)", maxlen - 1);
		return ft_numeric_64;

	case 10:
		strncpy(FieldName, "subdirs w/o access", maxlen - 1);
		return ft_boolean;

	case 11:
		strncpy(FieldName, "subdirs w/o access (count)", maxlen - 1);
		return ft_numeric_64;

	default:
		return ft_nomorefields;
	}
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	struct stat buf;

	if (strncmp(FileName + strlen(FileName) - 3, "/..", 4) == 0)
		return ft_fileerror;

	if (UnitIndex == 1 && lstat(FileName, &buf) != 0)
		return ft_fileerror;
	else if (UnitIndex == 0 && stat(FileName, &buf) != 0)
		return ft_fileerror;

	if (!S_ISDIR(buf.st_mode))
		return ft_fileerror;

	if (access(FileName, R_OK) != 0)
		return ft_fieldempty;

	switch (FieldIndex)
	{
	case 0:
		*(int*)FieldValue = emptychk(FileName, false);
		return ft_boolean;

	case 1:
		*(int*)FieldValue = emptychk(FileName, true);
		return ft_boolean;

	case 2:
		*(int64_t*)FieldValue = calccount(FileName, DT_DIR);
		break;

	case 3:
		*(int64_t*)FieldValue = calccount(FileName, DT_REG);
		break;

	case 4:
		*(int64_t*)FieldValue = calccount(FileName, DT_LNK);
		break;

	case 5:
		*(int64_t*)FieldValue = calccount(FileName, DT_FIFO);
		break;

	case 6:
		*(int64_t*)FieldValue = calccount(FileName, DT_SOCK);
		break;

	case 7:
		*(int64_t*)FieldValue = calccount(FileName, DT_BLK);
		break;

	case 8:
		*(int64_t*)FieldValue = calccount(FileName, DT_CHR);
		break;

	case 9:
		*(int64_t*)FieldValue = calccount(FileName, DT_UNKNOWN);
		break;

	case 10:
		*(int*)FieldValue = calcnoacess(FileName, true);
		return ft_boolean;

	case 11:
		*(int64_t*)FieldValue = calcnoacess(FileName, false);
		break;

	default:
		return ft_fieldempty;
	}

	return ft_numeric_64;
}
