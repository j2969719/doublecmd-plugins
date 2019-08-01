#include <stdbool.h>
#include <archive.h>
#include <archive_entry.h>
#include <string.h>
#include "wcxplugin.h"

tChangeVolProc gChangeVolProc;
tProcessDataProc gProcessDataProc;

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	int r = archive_read_open_filename(a, ArchiveData->ArcName, 10240);

	if (r != ARCHIVE_OK)
	{
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return 0;
	}

	return (HANDLE)a;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	struct archive_entry *entry;
	memset(HeaderData, 0, sizeof(HeaderData));

	if (archive_read_next_header(hArcData, &entry) == ARCHIVE_OK)
	{
		strncpy(HeaderData->FileName, archive_entry_pathname(entry), 259);
		HeaderData->PackSize = archive_entry_size(entry);
		HeaderData->UnpSize = archive_entry_size(entry);
		HeaderData->FileTime = archive_entry_mtime(entry);
		HeaderData->FileAttr = archive_entry_mode(entry);
		return 0;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	if (Operation == PK_EXTRACT)
	{
		ssize_t size;
		char buff[100];
		FILE *f = fopen(DestName, "wb");

		if (!f)
			return E_EWRITE;

		while ((size = archive_read_data(hArcData, buff, sizeof(buff))) > 0)
		{
			if (gProcessDataProc(DestName, 0) == 0)
			{
				fclose(f);
				return E_EABORTED;
			}

			fwrite(buff, 1, size, f);
		}

		fclose(f);
	}

	return 0;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	archive_read_free(hArcData);
	return 0;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	gProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{
	gChangeVolProc = pChangeVolProc1;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	int r = archive_read_open_filename(a, FileName, 10240);

	if (r != ARCHIVE_OK)
		return false;
	else
	{
		archive_read_free(a);
		return true;
	}
}

int DCPCALL GetPackerCaps()
{
	return PK_CAPS_BY_CONTENT;
}
