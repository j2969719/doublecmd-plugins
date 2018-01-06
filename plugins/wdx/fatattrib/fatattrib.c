#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/msdos_fs.h>
#include <string.h>
#include "wdxplugin.h"

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{

	switch(FieldIndex)
	{
		case 0:
			strncpy(FieldName, "hidden", maxlen-1);
			return ft_boolean;
		case 1:
			strncpy(FieldName, "readonly", maxlen-1);
			return ft_boolean;
		case 2:
			strncpy(FieldName, "system", maxlen-1);
			return ft_boolean;
		case 3:
			strncpy(FieldName, "archive", maxlen-1);
			return ft_boolean;
		default:
			return ft_nomorefields;
	}

}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{

	int fd;
	uint32_t attr;

	if (fd = open(FileName, O_RDONLY) == -1)
		return ft_fileerror;
	if (ioctl(fd, FAT_IOCTL_GET_ATTRIBUTES, &attr) == -1)
	{
		close(fd);
		return ft_fileerror;
	}
	close(fd);

	switch(FieldIndex)
	{
		case 0:
			if (attr & ATTR_HIDDEN)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
			break;
		case 1:
			if (attr & ATTR_RO)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
			break;
		case 2:
			if (attr & ATTR_SYS)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
			break;
		case 3:
			if (attr & ATTR_ARCH)
				*(int*)FieldValue = 1;
			else
				*(int*)FieldValue = 0;
			break;
		default:
			return ft_fieldempty;
	}
	return ft_boolean;

}

