#include <stdio.h>
#include <string.h>
#include "wdxplugin.h"

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex == 0)
	{
		snprintf(FieldName, maxlen - 1, "skip dotfiles");
		Units[0] = '\0';
		return ft_boolean;
	}
	else
		return ft_nomorefields;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (FieldIndex != 0)
		return ft_nosuchfield;

	if (strstr(FileName, "/.") != NULL)
		*(int*)FieldValue = 0;
	else
		*(int*)FieldValue = 1;

	return ft_boolean;
}
