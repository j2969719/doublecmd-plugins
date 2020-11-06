#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include "wdxplugin.h"


#define MAX_FIELDS 10

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex <= MAX_FIELDS)
	{
		snprintf(FieldName, maxlen - 1, "%d", FieldIndex + 1);
		strncpy(Units, "comma|newline", maxlen - 1);
		return ft_string;
	}
	else
		return ft_nomorefields;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	DIR *dir;
	char tmp[PATH_MAX];
	int count = 0;
	struct dirent *ent;

	if ((dir = opendir(FileName)) == NULL)
	{
		int errsv = errno;

		if (errsv == EACCES)
		{
			snprintf(FieldValue, maxlen - 1, "Permission denied");
			return ft_string;
		}
		else
			return ft_fileerror;
	}

	while ((ent = readdir(dir)) != NULL)
	{
		if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0))
			continue;

		if (count > 0)
		{
			strncpy(tmp, (char*)FieldValue, maxlen - 1);

			if (FieldIndex + 1 == count)

				snprintf((char*)FieldValue, maxlen - 1, "%s%s...", tmp, (UnitIndex == 1) ? "\n" : ", ");
			else

				snprintf((char*)FieldValue, maxlen - 1, "%s%s%s", tmp, (UnitIndex == 1) ? "\n" : ", ", ent->d_name);

		}
		else
			snprintf((char*)FieldValue, maxlen - 1, "%s", ent->d_name);

		if (FieldIndex + 1 == count++)
			break;
	}

	if (count == 0)
		snprintf(FieldValue, maxlen - 1, "Empty directory");

	closedir(dir);
	return ft_string;
}
