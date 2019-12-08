#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <string.h>
#include "wcxplugin.h"


HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
	return E_SUCCESS;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	return E_NOT_SUPPORTED;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{

}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{

}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	return false;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_HIDE;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	bool hardlinks = false;
	char path[PATH_MAX];
	char lnk_path[PATH_MAX];
	char pkdir[PATH_MAX];

	const char *ext = strrchr(PackedFile, '.');

	if (strcasecmp(ext, ".hardlinks") == 0)
		hardlinks = true;

	snprintf(pkdir, PATH_MAX, "%s/", dirname(PackedFile));

	while (*AddList)
	{
		if (AddList[strlen(AddList) - 1] != '/')
		{
			snprintf(lnk_path, PATH_MAX, "%s%s", pkdir, AddList);
			snprintf(path, PATH_MAX, "%s%s", SrcPath, AddList);

			if ((hardlinks && link(path, lnk_path) != 0) || (!hardlinks && symlink(path, lnk_path) != 0))
				return E_EWRITE;
		}
		else
		{
			snprintf(path, PATH_MAX, "%s%s", pkdir, AddList);

			if (access(path, F_OK) != 0)
			{
				if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
					return E_EWRITE;
			}
		}

		while (*AddList++);
	}
}
