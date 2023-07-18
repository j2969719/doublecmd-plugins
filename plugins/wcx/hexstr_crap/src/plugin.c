#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

#define PACKERCAPS PK_CAPS_NEW | PK_CAPS_SEARCHTEXT | PK_CAPS_BY_CONTENT | PK_CAPS_OPTIONS
#define LINE_MAX 256
#define FAKE_EXT ".hexstr"

#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox

#define FALSE 0
#define TRUE !FALSE

typedef struct sArcData
{
	BOOL end;
	char filename[PATH_MAX];
	char basename[PATH_MAX];
	tChangeVolProc ChangeVolProc;
	tProcessDataProc ProcessDataProc;
} tArcData;

typedef tArcData* ArcData;
typedef void *HINSTANCE;

tChangeVolProc gChangeVolProc  = NULL;
tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;
int gBuffSize = 32;

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	tArcData *data = (tArcData *)malloc(sizeof(tArcData));

	if (data == NULL)
	{
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	memset(data, 0, sizeof(tArcData));
	snprintf(data->filename, PATH_MAX, "%s", ArchiveData->ArcName);
	snprintf(data->basename, PATH_MAX, "%s", basename(data->filename));
	char *pos = strrchr(data->basename, '.');

	if (pos)
		*pos = '\0';

	return data;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	ArcData data = (ArcData)hArcData;

	if (data->end == FALSE)
	{
		struct stat buf;
		memset(HeaderDataEx, 0, sizeof(&HeaderDataEx));
		snprintf(HeaderDataEx->FileName, sizeof(HeaderDataEx->FileName) - 1, "%s", data->basename);

		FILE *fp;
		ssize_t line_size = 1;

		if ((fp = fopen(data->filename, "r")) != NULL)
		{
			size_t len = 0;
			char *line = NULL;

			line_size = getline(&line, &len, fp);

			if (line_size <= 0)
				line_size = 1;

			free(line);
			fclose(fp);
		}

		if (stat(data->filename, &buf) == 0)
		{
			HeaderDataEx->FileAttr = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
			HeaderDataEx->PackSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
			HeaderDataEx->PackSize = buf.st_size & 0x00000000FFFFFFFF;
			HeaderDataEx->UnpSizeHigh = (((buf.st_size - buf.st_size / line_size) / 2) & 0xFFFFFFFF00000000) >> 32;
			HeaderDataEx->UnpSize = ((buf.st_size - buf.st_size / line_size) / 2) & 0x00000000FFFFFFFF;
			HeaderDataEx->FileTime = buf.st_mtime;
		}

		return E_SUCCESS;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	FILE *fp;
	int result = E_SUCCESS;
	ArcData data = (ArcData)hArcData;

	if (Operation == PK_EXTRACT && (fp = fopen(data->filename, "r")) != NULL)
	{
		int fd = open(DestName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if (fd > -1)
		{
			ssize_t lread;
			size_t len = 0;
			char *line = NULL;

			while ((lread = getline(&line, &len, fp)) != -1)
			{
				if (line[lread - 1] == '\n')
					line[lread - 1] = '\0';

				unsigned int val;
				int size = (int)strlen(line) / 2;
				unsigned char *buff = malloc(sizeof(unsigned char) * size);
				memset(buff, 0, size);

				for (int i = 0; i < size; i++)
				{
					sscanf(line + i * 2, "%02X", &val);
					buff[i] = (unsigned char)val;
				}

				if (write(fd, buff, size) == -1)
				{
					result = E_EWRITE;
					remove(DestName);
					free(buff);
					break;
				}

				free(buff);

				if (data->ProcessDataProc && data->ProcessDataProc(DestName, size) == 0)
				{
					result = E_EABORTED;
					remove(DestName);
					break;
				}
			}

			free(line);

			close(fd);
		}

		fclose(fp);
	}

	data->end = TRUE;
	return result;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData data = (ArcData)hArcData;

	free(data);

	return E_SUCCESS;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData data = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		gProcessDataProc = pProcessDataProc;
	else
		data->ProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc)
{
	ArcData data = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		gChangeVolProc = pChangeVolProc;
	else
		data->ChangeVolProc = pChangeVolProc;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	char *dot = strrchr(FileName, '.');

	if (strcmp(dot, FAKE_EXT) == 0)
		return TRUE;

	return FALSE;
}

int DCPCALL GetPackerCaps(void)
{
	return PACKERCAPS;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	char buff[256];
	snprintf(buff, sizeof(buff), "%d", gBuffSize);

	while (InputBox(NULL, "Line size (bytes)", FALSE, buff, sizeof(buff) - 1) != FALSE)
	{
		int result = atoi(buff);

		if (result > 0 && result < LINE_MAX)
		{
			gBuffSize = result;
			break;
		}
		else
			MessageBox("Invalid value.", NULL,  MB_OK | MB_ICONERROR);
	}
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	ssize_t len;
	char path[PATH_MAX];
	int result = E_SUCCESS;


	unsigned char *buff = malloc(sizeof(unsigned char) * gBuffSize);
	memset(buff, 0, gBuffSize);

	while (*AddList)
	{
		if (AddList[strlen(AddList) - 1] != '/')
		{
			snprintf(path, PATH_MAX, "%s%s", SrcPath, AddList);
			int fd = open(path, O_RDONLY);

			if (fd == -1)
				return E_EREAD;

			FILE *fp = fopen(PackedFile, "w+");

			if (fp)
			{

				while ((len = read(fd, buff, gBuffSize)) > 0)
				{
					for (int i = 0; i < len; i++)
						fprintf(fp, "%02X", buff[i]);

					fprintf(fp, "\n");

					if (gProcessDataProc && gProcessDataProc(path, len) == 0)
					{
						result = E_EABORTED;
						remove(PackedFile);
						break;
					}
				}

				fclose(fp);
			}
			else
				result = E_EWRITE;

			close(fd);
		}

		while (*AddList++);
	}

	free(buff);

	return result;
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = (tExtensionStartupInfo*)malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
	{
		free(gExtensions);
	}

	gExtensions = NULL;
}
