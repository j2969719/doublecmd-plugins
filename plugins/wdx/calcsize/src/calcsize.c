#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "wdxplugin.h"

static int64_t calcdirszie(const char *name)
{
	DIR *dir;
	int64_t size = 0;
	struct dirent *ent;
	struct stat buf;

	if ((dir = opendir(name)) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
			{
				if (ent->d_type == DT_REG)
				{
					char file[PATH_MAX];
					snprintf(file, PATH_MAX, "%s/%s", name, ent->d_name);

					if (stat(file, &buf) == 0)
						size += buf.st_size;
				}

				if (ent->d_type == DT_DIR)
				{
					char path[PATH_MAX];
					snprintf(path, PATH_MAX, "%s/%s", name, ent->d_name);
					size += calcdirszie(path);
				}
			}
		}

		closedir(dir);
	}

	return size;
}

static double sizecnv(int64_t size, int bytes, int bpow)
{
	double pb = pow(bytes, bpow);
	return round(size / pb * 10) / 10;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	strncpy(Units, "default|files only|dirs only", maxlen - 1);

	switch (FieldIndex)
	{
	case 0:
		strncpy(FieldName, "bytes", maxlen - 1);
		return ft_numeric_64;

	case 1:
		strncpy(FieldName, "K", maxlen - 1);
		return ft_numeric_floating;

	case 2:
		strncpy(FieldName, "M", maxlen - 1);
		return ft_numeric_floating;

	case 3:
		strncpy(FieldName, "G", maxlen - 1);
		return ft_numeric_floating;

	case 4:
		strncpy(FieldName, "T", maxlen - 1);
		return ft_numeric_floating;

	case 5:
		strncpy(FieldName, "kB", maxlen - 1);
		return ft_numeric_floating;

	case 6:
		strncpy(FieldName, "MB", maxlen - 1);
		return ft_numeric_floating;

	case 7:
		strncpy(FieldName, "GB", maxlen - 1);
		return ft_numeric_floating;

	case 8:
		strncpy(FieldName, "TB", maxlen - 1);
		return ft_numeric_floating;

	default:
		return ft_nomorefields;
	}
}


int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	struct stat buf;
	int64_t rsize;

	if (strncmp(FileName + strlen(FileName) - 3, "/..", 4) == 0)
		return ft_fileerror;

	if (lstat(FileName, &buf) != 0)
		return ft_fileerror;

	if (S_ISDIR(buf.st_mode) && UnitIndex != 1)
		rsize = calcdirszie(FileName);
	else if (!S_ISDIR(buf.st_mode) && UnitIndex != 2)
		rsize = buf.st_size;
	else
		return ft_fieldempty;

	if (S_ISDIR(buf.st_mode) && access(FileName, R_OK) != 0)
		return ft_fieldempty;

	switch (FieldIndex)
	{
	case 0:
		*(int64_t*)FieldValue = rsize;
		return ft_numeric_64;

	case 1:
		*(double*)FieldValue = sizecnv(rsize, 1024, 1);
		break;

	case 2:
		*(double*)FieldValue = sizecnv(rsize, 1024, 2);
		break;

	case 3:
		*(double*)FieldValue = sizecnv(rsize, 1024, 3);
		break;

	case 4:
		*(double*)FieldValue = sizecnv(rsize, 1024, 4);
		break;

	case 5:
		*(double*)FieldValue = sizecnv(rsize, 1000, 1);
		break;

	case 6:
		*(double*)FieldValue = sizecnv(rsize, 1000, 2);
		break;

	case 7:
		*(double*)FieldValue = sizecnv(rsize, 1000, 3);
		break;

	case 8:
		*(double*)FieldValue = sizecnv(rsize, 1000, 4);
		break;

	default:
		return ft_fieldempty;
	}

	return ft_numeric_floating;
}
