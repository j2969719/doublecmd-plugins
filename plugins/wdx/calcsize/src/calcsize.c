#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "wdxplugin.h"

typedef struct
{
	char *name;
	int type;
	char *units;
} ContentField;

enum
{
	FIELD_BYTES,
	FIELD_KiB,
	FIELD_MiB,
	FIELD_GiB,
	FIELD_TiB,
	FIELD_kB,
	FIELD_MB,
	FIELD_GB,
	FIELD_TB,
	FIELDS,

// https://www.youtube.com/watch?v=cSAp9sBzPbc
	FIELD_HUMAN,
};

static const double KiB = 1024;
static const double MiB = 1024 * 1024;
static const double GiB = 1024 * 1024 * 1024;
static const double TiB = 1024 * GiB;

static const double kB = 1000;
static const double MB = 1000 * 1000;
static const double GB = 1000 * 1000 * 1000;
static const double TB = 1000 * GB;

static const ContentField fields[] =
{
	[FIELD_BYTES] = {"bytes",	ft_numeric_64,		"default|files only|dirs only"},
	[FIELD_KiB]   = {"K",		ft_numeric_floating,	"default|files only|dirs only"},
	[FIELD_MiB]   = {"M",		ft_numeric_floating,	"default|files only|dirs only"},
	[FIELD_GiB]   = {"G",		ft_numeric_floating,	"default|files only|dirs only"},
	[FIELD_TiB]   = {"T",		ft_numeric_floating,	"default|files only|dirs only"},
	[FIELD_kB]    = {"kB",		ft_numeric_floating,	"default|files only|dirs only"},
	[FIELD_MB]    = {"MB",		ft_numeric_floating,	"default|files only|dirs only"},
	[FIELD_GB]    = {"GB",		ft_numeric_floating,	"default|files only|dirs only"},
	[FIELD_TB]    = {"TB",		ft_numeric_floating,	"default|files only|dirs only"},
//	[FIELD_HUMAN] = {"size",	ft_numeric_floating,	"default|files only|dirs only"},
};

static int64_t calc_dir_size(const char *path)
{
	DIR *dir;
	int64_t size = 0;
	struct dirent *ent;
	struct stat buf;

	if ((dir = opendir(path)) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
			{
				char filename[PATH_MAX];
				snprintf(filename, PATH_MAX, "%s/%s", path, ent->d_name);

				if (ent->d_type == DT_REG)
				{
					if (stat(filename, &buf) == 0)
						size += buf.st_size;
				}
				else if (ent->d_type == DT_DIR)
					size += calc_dir_size(filename);
			}
		}
		closedir(dir);
	}

	return size;
}

static void fill_human_size(int64_t size, char *buf, int maxlen)
{
	if (size < KiB)
		snprintf(buf, maxlen, "%ld", size);
	else if (size < MiB)
		snprintf(buf, maxlen, "%.2f KB", size / KiB);
	else if (size < GiB)
		snprintf(buf, maxlen, "%.2f MB", size / MiB);
	else if (size < TiB)
		snprintf(buf, maxlen, "%.2f GB", size / GiB);
	else
		snprintf(buf, maxlen, "%'.2f TB", size / TiB);
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= FIELDS)
		return ft_nomorefields;

	snprintf(FieldName, maxlen - 1, "%s", fields[FieldIndex].name);
	snprintf(Units, maxlen - 1, "%s", fields[FieldIndex].units);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	struct stat buf;
	int64_t size;
	size_t len = strlen(FileName);

	if (len >= 3 && strcmp(FileName + len - 3, "/..") == 0)
		return ft_fileerror;

	if (lstat(FileName, &buf) != 0)
		return ft_fileerror;

	if (S_ISDIR(buf.st_mode) && UnitIndex != 1)
		size = calc_dir_size(FileName);
	else if (!S_ISDIR(buf.st_mode) && UnitIndex != 2)
		size = buf.st_size;
	else
		return ft_fieldempty;

	if (S_ISDIR(buf.st_mode) && access(FileName, R_OK) != 0)
		return ft_fieldempty;

	switch (FieldIndex)
	{
	case FIELD_BYTES:
		*(int64_t*)FieldValue = size;
		break;

	case FIELD_KiB:
		*(double*)FieldValue = round(size / KiB * 10) / 10;
		break;

	case FIELD_MiB:
		*(double*)FieldValue = round(size / MiB * 10) / 10;
		break;

	case FIELD_GiB:
		*(double*)FieldValue = round(size / GiB * 10) / 10;
		break;

	case FIELD_TiB:
		*(double*)FieldValue = round(size / TiB * 10) / 10;
		break;

	case FIELD_kB:
		*(double*)FieldValue = round(size / kB * 10) / 10;
		break;

	case FIELD_MB:
		*(double*)FieldValue = round(size / MB * 10) / 10;
		break;

	case FIELD_GB:
		*(double*)FieldValue = round(size / GB * 10) / 10;
		break;

	case FIELD_TB:
		*(double*)FieldValue = round(size / TB * 10) / 10;
		break;

	case FIELD_HUMAN:
		*(double*)FieldValue = (double)size;
		fill_human_size(size, (char*)FieldValue + sizeof(double), maxlen - sizeof(double) - 1);
		break;

	default:
		return ft_fieldempty;
	}

	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "%s", DETECT_STRING);
	return 0;
}
