#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <math.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <string.h>
#include "wdxplugin.h"

uint64_t calcdirszie(const char *name)
{
	DIR *dir;
	uint64_t size = 0;
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
					sprintf(file, "%s/%s", name, ent->d_name);
					if (stat(file, &buf) == 0)
						size += buf.st_size;
				}
				if (ent->d_type == DT_DIR)
				{
					char path[PATH_MAX];
					sprintf(path, "%s/%s", name, ent->d_name);
					size += calcdirszie(path);
				}
			}
		}
		closedir(dir);
	}
	return size;
}

double sizecnv(uint64_t size, int bytes, int bpow)
{
	double pb = pow(bytes, bpow);
	return round(size / pb * 10) / 10; 
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{

	switch(FieldIndex)
	{
		case 0:
			strncpy(FieldName, "bytes", maxlen-1);
			return ft_numeric_64;
		case 1:
			strncpy(FieldName, "K", maxlen-1);
			return ft_numeric_floating;
		case 2:
			strncpy(FieldName, "M", maxlen-1);
			return ft_numeric_floating;
		case 3:
			strncpy(FieldName, "G", maxlen-1);
			return ft_numeric_floating;
		case 4:
			strncpy(FieldName, "T", maxlen-1);
			return ft_numeric_floating;
		case 5:
			strncpy(FieldName, "kB", maxlen-1);
			return ft_numeric_floating;
		case 6:
			strncpy(FieldName, "MB", maxlen-1);
			return ft_numeric_floating;
		case 7:
			strncpy(FieldName, "GB", maxlen-1);
			return ft_numeric_floating;
		case 8:
			strncpy(FieldName, "TB", maxlen-1);
			return ft_numeric_floating;
		default:
			return ft_nomorefields;
	}
}


int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	struct stat buf;
	uint64_t rsize;
	double val;

	if (lstat(FileName, &buf) != 0)
		return ft_fileerror;
	if (S_ISDIR(buf.st_mode))
		rsize = calcdirszie(FileName);
	else
		rsize = buf.st_size;

	switch(FieldIndex)
	{
		case 0:
			*(uint64_t*)FieldValue = rsize;
			return ft_numeric_64;
		case 1:
			val = sizecnv(rsize, 1024, 1);
			if (val == 0)
				return ft_fieldempty;
			else
				*(double*)FieldValue = val;
			break;
		case 2:
			val = sizecnv(rsize, 1024, 2);
			if (val == 0)
				return ft_fieldempty;
			else
				*(double*)FieldValue = val;
			break;
		case 3:
			val = sizecnv(rsize, 1024, 3);
			if (val == 0)
				return ft_fieldempty;
			else
				*(double*)FieldValue = val;
			break;
		case 4:
			val = sizecnv(rsize, 1024, 4);
			if (val == 0)
				return ft_fieldempty;
			else
				*(double*)FieldValue = val;
			break;
		case 5:
			val = sizecnv(rsize, 1000, 1);
			if (val == 0)
				return ft_fieldempty;
			else
				*(double*)FieldValue = val;
			break;
		case 6:
			val = sizecnv(rsize, 1000, 2);
			if (val == 0)
				return ft_fieldempty;
			else
				*(double*)FieldValue = val;
			break;
		case 7:
			val = sizecnv(rsize, 1000, 3);
			if (val == 0)
				return ft_fieldempty;
			else
				*(double*)FieldValue = val;
			break;
		case 8:
			val = sizecnv(rsize, 1000, 4);
			if (val == 0)
				return ft_fieldempty;
			else
				*(double*)FieldValue = val;
			break;
		default:
			return ft_fieldempty;
	}
	return ft_numeric_floating;
}

