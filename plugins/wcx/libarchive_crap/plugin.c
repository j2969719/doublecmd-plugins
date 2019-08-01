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
		return E_SUCCESS;
	}

	return (HANDLE)handle;
}
int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderDate)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(ArcData hArcData, tHeaderDataEx *HeaderDataEx)
{
	int64_t size;
	memset(HeaderDataEx, 0, sizeof(HeaderDataEx));

	if (archive_read_next_header(hArcData->archive, &hArcData->entry) == ARCHIVE_OK)
	{
		strncpy(HeaderDataEx->FileName, archive_entry_pathname(hArcData->entry), sizeof(HeaderDataEx->FileName) - 1);
		size = archive_entry_size(hArcData->entry);
		HeaderDataEx->PackSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->PackSize = size & 0x00000000FFFFFFFF;
		HeaderDataEx->UnpSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->UnpSize = size & 0x00000000FFFFFFFF;
		HeaderDataEx->FileTime = archive_entry_mtime(hArcData->entry);
		HeaderDataEx->FileAttr = archive_entry_mode(hArcData->entry);

		if (archive_entry_is_encrypted(hArcData->entry))
			HeaderDataEx->Flags |= RHDF_ENCRYPTED;

		return E_SUCCESS;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(ArcData hArcData, int Operation, char *DestPath, char *DestName)
{
	int ret;
	int result = E_SUCCESS;
	size_t size;
	la_int64_t offset;
	const void *buff;

	if (Operation == PK_EXTRACT)
	{
		int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_FFLAGS;

		if (archive_entry_is_encrypted(hArcData->entry))
			return E_NOT_SUPPORTED;

		struct archive *a = archive_write_disk_new();
		archive_entry_set_pathname(hArcData->entry, DestName);
		archive_write_disk_set_options(a, flags);
		archive_write_disk_set_standard_lookup(a);
		archive_write_header(a, hArcData->entry);

		while ((ret = archive_read_data_block(hArcData->archive, &buff, &size, &offset)) != ARCHIVE_EOF)
		{
			if (ret < ARCHIVE_OK)
			{
				printf("%s\n", archive_error_string(hArcData->archive));
				result = E_EREAD;
				break;
			}
			else if (archive_write_data_block(a, buff, size, offset) < ARCHIVE_OK)
			{
				printf("%s\n", archive_error_string(a));
				result = E_EWRITE;
				break;
			}
			else if (hArcData->gProcessDataProc(DestName, 0) == 0)
			{
				result = E_EABORTED;
				break;
			}
		}

		archive_write_finish_entry(a);
		archive_write_close(a);
		archive_write_free(a);
	}

	return result;
}

int DCPCALL CloseArchive(ArcData hArcData)
{
	archive_read_close(hArcData->archive);
	archive_read_free(hArcData->archive);
	free(hArcData);
	return E_SUCCESS;
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
	archive_read_free(a);

	if (r != ARCHIVE_OK)
		return false;
	else
		return true;
}

int DCPCALL GetPackerCaps()
{
	return PK_CAPS_BY_CONTENT;
}
