#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <archive.h>
#include <archive_entry.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <errno.h>

#define errmsg(msg) gStartupInfo->MessageBox((char*)msg, NULL, MB_OK | MB_ICONERROR);

typedef struct sArcData
{
	struct archive *archive;
	struct archive_entry *entry;
	tChangeVolProc gChangeVolProc;
	tProcessDataProc gProcessDataProc;
} tArcData;

typedef tArcData* ArcData;

tChangeVolProc gChangeVolProc;
tProcessDataProc gProcessDataProc;
tExtensionStartupInfo* gStartupInfo;

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	gStartupInfo = malloc(sizeof(tExtensionStartupInfo));
	memcpy(gStartupInfo, StartupInfo, sizeof(tExtensionStartupInfo));
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	free(gStartupInfo);
}


HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	tArcData * handle;
	handle = malloc(sizeof(tArcData));
	memset(handle, 0, sizeof(tArcData));
	handle->archive = archive_read_new();
	archive_read_support_filter_all(handle->archive);
	archive_read_support_format_raw(handle->archive);
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

		//if (archive_entry_is_encrypted(hArcData->entry))
			//return E_NOT_SUPPORTED;

		struct archive *a = archive_write_disk_new();
		archive_entry_set_pathname(hArcData->entry, DestName);
		archive_write_disk_set_options(a, flags);
		archive_write_disk_set_standard_lookup(a);
		archive_write_header(a, hArcData->entry);

		while ((ret = archive_read_data_block(hArcData->archive, &buff, &size, &offset)) != ARCHIVE_EOF)
		{
			if (ret < ARCHIVE_OK)
			{
				//printf("%s\n", archive_error_string(hArcData->archive));
				//result = E_EREAD;
				errmsg(archive_error_string(hArcData->archive));
				result = E_EABORTED;
				break;
			}
			else if (archive_write_data_block(a, buff, size, offset) < ARCHIVE_OK)
			{
				//printf("%s\n", archive_error_string(a));
				//result = E_EWRITE;
				errmsg(archive_error_string(a));
				result = E_EABORTED;
				break;
			}
			else if (hArcData->gProcessDataProc(DestName, size) == 0)
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
	if ((int)(long)hArcData == -1)
		gProcessDataProc = pProcessDataProc;
	else
		hArcData->gProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(ArcData hArcData, tChangeVolProc pChangeVolProc1)
{
	if ((int)(long)hArcData == -1)
		gChangeVolProc = pChangeVolProc1;
	else
		hArcData->gChangeVolProc = pChangeVolProc1;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	//archive_read_support_format_raw(a);
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
	return PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_SEARCHTEXT | PK_CAPS_BY_CONTENT;
}

void DCPCALL ConfigurePacker(HWND Parent, void *DllInstance)
{
	gStartupInfo->MessageBox((char*)archive_version_details(), NULL, MB_OK | MB_ICONINFORMATION);
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	struct archive_entry *entry;
	struct stat st;
	char buff[8192];
	int fd, ret;
	ssize_t len;
	char infile[PATH_MAX];
	char pkfile[PATH_MAX];
	int result = E_SUCCESS;

	if (access(PackedFile, F_OK) != -1)
		return E_NOT_SUPPORTED;

	const char *ext = strrchr(PackedFile, '.');

	struct archive *a = archive_write_new();

	if (strcmp(ext, ".tzst") == 0)
	{
		ret = archive_write_set_format_pax_restricted(a);
		ret = archive_write_add_filter_zstd(a);
	}
	else if (strcmp(ext, ".zst") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_zstd(a);
	}
	else if (strcmp(ext, ".lz4") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lz4(a);
	}
	else if (strcmp(ext, ".lz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lzip(a);
	}
	else if (strcmp(ext, ".lzo") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lzop(a);
	}
	else if (strcmp(ext, ".lrz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lrzip(a);
	}
	else if (strcmp(ext, ".grz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_grzip(a);
	}
	else if (strcmp(ext, ".b64") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_b64encode(a);
	}
	else if (strcmp(ext, ".uue") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_uuencode(a);
	}
	else  
		ret = archive_write_set_format_filter_by_ext(a, PackedFile);

	if (ret < ARCHIVE_OK)
	{
		errmsg(archive_error_string(a));
		archive_write_free(a);
		return 0;
	}

	archive_write_open_filename(a, PackedFile);
	struct archive *disk = archive_read_disk_new();
	archive_read_disk_set_standard_lookup(disk);
	archive_read_disk_set_symlink_physical(disk);

	while (*AddList)
	{

		strcpy(infile, SrcPath);
		char* pos = strrchr(infile, '/');

		if (pos != NULL)
			strcpy(pos + 1, AddList);

		if (!SubPath)
			strcpy(pkfile, AddList);
		else
		{
			strcpy(pkfile, SrcPath);
			pos = strrchr(pkfile, '/');

			if (pos != NULL)
				strcpy(pos + 1, AddList);
		}

		lstat(infile, &st);

		entry = archive_entry_new();

		fd = open(infile, O_RDONLY);

		if (fd != -1)
		{
			archive_entry_set_pathname(entry, infile);
			archive_entry_copy_stat(entry, &st);
			archive_read_disk_entry_from_file(disk, entry, fd, &st);
			archive_entry_set_pathname(entry, pkfile);
			archive_write_header(a, entry);

			while ((len = read(fd, buff, sizeof(buff))) > 0)
			{
				if (archive_write_data(a, buff, len) < ARCHIVE_OK)
				{
					errmsg(archive_error_string(a));
					result = E_EWRITE;
					break;
				}

				if (gProcessDataProc(infile, len) == 0)
				{
					result = E_EABORTED;
					break;
				}
			}

			close(fd);
		}
		else
		{
			int errsv = errno;
			char *msg;
			asprintf(&msg, "%s: %s", infile, strerror(errsv));
			errmsg(msg);
			free(msg);
			//result = E_EREAD;
		}

		if (result != E_SUCCESS)
			break;

		while (*AddList++);
	}

	archive_read_free(disk);
	archive_entry_free(entry);
	archive_write_finish_entry(a);
	archive_write_close(a);
	archive_write_free(a);

	return result;
}
