#include <stdlib.h>
#include <stdbool.h>
#include <archive.h>
#include <archive_entry.h>
#include <string.h>
#include "wcxplugin.h"

typedef struct sArcData
{
	struct archive *archive;
	struct archive_entry *entry;
	tChangeVolProc gChangeVolProc;
	tProcessDataProc gProcessDataProc;
} tArcData;

typedef tArcData* ArcData;

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	tArcData * handle;
	handle = malloc(sizeof(tArcData));
	memset(handle, 0, sizeof(tArcData));
	handle->archive = archive_read_new();
	archive_read_support_filter_all(handle->archive);
	archive_read_support_format_all(handle->archive);
	int r = archive_read_open_filename(handle->archive, ArchiveData->ArcName, 10240);

	if (r != ARCHIVE_OK)
	{
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return 0;
	}

	return (HANDLE)handle;
}

int DCPCALL ReadHeader(ArcData hArcData, tHeaderData *HeaderData)
{
	memset(HeaderData, 0, sizeof(HeaderData));

	if (archive_read_next_header(hArcData->archive, &hArcData->entry) == ARCHIVE_OK)
	{
		strncpy(HeaderData->FileName, archive_entry_pathname(hArcData->entry), 259);
		HeaderData->PackSize = archive_entry_size(hArcData->entry);
		HeaderData->UnpSize = archive_entry_size(hArcData->entry);
		HeaderData->FileTime = archive_entry_mtime(hArcData->entry);
		HeaderData->FileAttr = archive_entry_mode(hArcData->entry);
		return 0;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(ArcData hArcData, int Operation, char *DestPath, char *DestName)
{
	size_t size;
	la_int64_t offset;
	const void *buff;

	if (hArcData->gProcessDataProc(DestName, 0) == 0)
		return E_EABORTED;

	if (Operation == PK_EXTRACT)
	{
		struct archive *a = archive_write_disk_new();
		int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_FFLAGS;
		archive_entry_set_pathname(hArcData->entry, DestName);
		archive_write_disk_set_options(a, flags);
		archive_write_disk_set_standard_lookup(a);
		archive_write_header(a, hArcData->entry);

		while (archive_read_data_block(hArcData->archive, &buff, &size, &offset) != ARCHIVE_EOF)
		{
			if (archive_write_data_block(a, buff, size, offset) < ARCHIVE_OK)
			{
				printf("%s\n", archive_error_string(a));
				archive_write_finish_entry(a);
				archive_write_close(a);
				archive_write_free(a);
				return E_EWRITE;
			}
		}

		archive_write_finish_entry(a);
		archive_write_close(a);
		archive_write_free(a);
	}

	return 0;
}

int DCPCALL CloseArchive(ArcData hArcData)
{
	archive_read_close(hArcData->archive);
	return 0;
}

void DCPCALL SetProcessDataProc(ArcData hArcData, tProcessDataProc pProcessDataProc)
{
	hArcData->gProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(ArcData hArcData, tChangeVolProc pChangeVolProc1)
{
	hArcData->gChangeVolProc = pChangeVolProc1;
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
