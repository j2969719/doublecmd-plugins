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
#include <pwd.h>
#include <grp.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

typedef struct sArcData
{
	struct archive *archive;
	struct archive_entry *entry;
	char arcname[PATH_MAX + 1];
	tChangeVolProc gChangeVolProc;
	tProcessDataProc gProcessDataProc;
} tArcData;

typedef tArcData* ArcData;

typedef void *HINSTANCE;

tChangeVolProc gChangeVolProc  = NULL;
tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gStartupInfo;

static char options[PATH_MAX];
static unsigned char lizard_magic[] = { 0x06, 0x22, 0x4D, 0x18 };

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	gStartupInfo = malloc(sizeof(tExtensionStartupInfo));
	memcpy(gStartupInfo, StartupInfo, sizeof(tExtensionStartupInfo));
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	free(gStartupInfo);
}

static int errmsg(const char *msg, long flags)
{
	return gStartupInfo->MessageBox(msg ? (char*)msg : "Unknown error", NULL, flags);
}

static bool mtree_opts_nodata(void)
{
	bool result = true;

	if (options[0] != '\0')
	{
		if (strstr(options, "all") != NULL)
			result = false;
		else if (strstr(options, "cksum") != NULL)
			result = false;
		else if (strstr(options, "md5") != NULL)
			result = false;
		else if (strstr(options, "rmd160") != NULL)
			result = false;
		else if (strstr(options, "sha1") != NULL)
			result = false;
		else if (strstr(options, "sha256") != NULL)
			result = false;
		else if (strstr(options, "sha384") != NULL)
			result = false;
		else if (strstr(options, "sha512") != NULL)
			result = false;
	}

	return result;
}

const char *archive_password_cb(struct archive *a, void *data)
{
	static char pass[PATH_MAX];

	if (gStartupInfo->InputBox("Double Commander", "Please enter the password:", true, pass, PATH_MAX - 1))
		return pass;
	else
		return NULL;
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	tArcData * handle;
	handle = malloc(sizeof(tArcData));
	memset(handle, 0, sizeof(tArcData));
	handle->archive = archive_read_new();

	if (archive_read_set_passphrase_callback(handle->archive, NULL, archive_password_cb) < ARCHIVE_OK)
		errmsg(archive_error_string(handle->archive), MB_OK | MB_ICONERROR);

	archive_read_support_filter_all(handle->archive);
	archive_read_support_filter_program_signature(handle->archive, "lizard -d", lizard_magic, sizeof(lizard_magic));
	archive_read_support_format_raw(handle->archive);
	archive_read_support_format_all(handle->archive);
	strncpy(handle->arcname, ArchiveData->ArcName, PATH_MAX);
	int r = archive_read_open_filename(handle->archive, ArchiveData->ArcName, 10240);

	if (r != ARCHIVE_OK)
	{
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

	return (HANDLE)handle;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	int ret;
	int64_t size;
	memset(HeaderDataEx, 0, sizeof(HeaderDataEx));
	ArcData handle = (ArcData)hArcData;

	while ((ret = archive_read_next_header(handle->archive, &handle->entry)) == ARCHIVE_RETRY)
	{
		if (errmsg(archive_error_string(handle->archive),
		                MB_RETRYCANCEL | MB_ICONWARNING) == ID_CANCEL)
			return E_EABORTED;
	}

	if (ret == ARCHIVE_FATAL)
	{
		errmsg(archive_error_string(handle->archive), MB_OK | MB_ICONERROR);
		return E_BAD_ARCHIVE;
	}

	if (ret != ARCHIVE_EOF)
	{
		if (ret == ARCHIVE_WARN)
			printf("libarchive: %s\n", archive_error_string(handle->archive));

		if (archive_format(handle->archive) == ARCHIVE_FORMAT_RAW)
		{
			char *filename = basename(handle->arcname);
			char *dot = strrchr(filename, '.');

			if (dot != NULL)
				*dot = '\0';

			strncpy(HeaderDataEx->FileName, filename, sizeof(HeaderDataEx->FileName) - 1);
		}
		else
		{
			strncpy(HeaderDataEx->FileName, archive_entry_pathname(handle->entry), sizeof(HeaderDataEx->FileName) - 1);
			size = archive_entry_size(handle->entry);
			HeaderDataEx->PackSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
			HeaderDataEx->PackSize = size & 0x00000000FFFFFFFF;
			HeaderDataEx->UnpSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
			HeaderDataEx->UnpSize = size & 0x00000000FFFFFFFF;
			HeaderDataEx->FileTime = archive_entry_mtime(handle->entry);
			HeaderDataEx->FileAttr = archive_entry_mode(handle->entry);

			if (archive_entry_is_encrypted(handle->entry))
				HeaderDataEx->Flags |= RHDF_ENCRYPTED;
		}

		return E_SUCCESS;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	int ret;
	int result = E_SUCCESS;
	size_t size;
	la_int64_t offset;
	const void *buff;
	ArcData handle = (ArcData)hArcData;

	if (Operation == PK_EXTRACT)
	{
		int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_FFLAGS;

		struct archive *a = archive_write_disk_new();
		archive_entry_set_pathname(handle->entry, DestName);
		archive_write_disk_set_options(a, flags);
		archive_write_disk_set_standard_lookup(a);
		archive_write_header(a, handle->entry);

		while ((ret = archive_read_data_block(handle->archive, &buff, &size, &offset)) != ARCHIVE_EOF)
		{
			if (ret < ARCHIVE_OK)
			{
				//printf("libarchive: %s\n", archive_error_string(handle->archive));
				//result = E_EREAD;
				errmsg(archive_error_string(handle->archive), MB_OK | MB_ICONERROR);
				result = E_EABORTED;
				break;
			}
			else if (archive_write_data_block(a, buff, size, offset) < ARCHIVE_OK)
			{
				//printf("libarchive: %s\n", archive_error_string(a));
				//result = E_EWRITE;
				errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
				result = E_EABORTED;
				break;
			}
			else if (handle->gProcessDataProc && handle->gProcessDataProc(DestName, size) == 0)
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

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData handle = (ArcData)hArcData;
	archive_read_close(handle->archive);
	archive_read_free(handle->archive);
	free(handle);
	return E_SUCCESS;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData handle = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !handle)
		gProcessDataProc = pProcessDataProc;
	else
		handle->gProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{
	ArcData handle = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !handle)
		gChangeVolProc = pChangeVolProc1;
	else
		handle->gChangeVolProc = pChangeVolProc1;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_filter_program_signature(a, "lizard -d", lizard_magic, sizeof(lizard_magic));
	//archive_read_support_format_raw(a);
	archive_read_support_format_all(a);
	int r = archive_read_open_filename(a, FileName, 10240);
	archive_read_free(a);

	if (r != ARCHIVE_OK)
		return false;
	else
		return true;
}

int DCPCALL GetPackerCaps(void)
{
	//return PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_SEARCHTEXT | PK_CAPS_BY_CONTENT;
	return PK_CAPS_NEW | PK_CAPS_SEARCHTEXT | PK_CAPS_BY_CONTENT;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	char *msg;
	asprintf(&msg, "%s\nOptions ($ man archive_write_set_options):", archive_version_details());
	gStartupInfo->InputBox("Double Commander", msg, false, options, PATH_MAX - 1);
	free(msg);
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	struct archive_entry *entry;
	struct stat st;
	char buff[8192];
	int fd, ret, id;
	ssize_t len;
	char infile[PATH_MAX];
	char pkfile[PATH_MAX];
	char link[PATH_MAX + 1];
	int result = E_SUCCESS;
	char *msg;

	struct passwd *pw;
	struct group  *gr;

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
	else if (strcmp(ext, ".mtree") == 0)
	{
		ret = archive_write_set_format_mtree(a);
		//ret = archive_write_set_format_mtree_classic(a);
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
	else if (strcmp(ext, ".lzma") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lzma(a);
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
	else if (strcmp(ext, ".liz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_program(a, "lizard");
	}
	else
		ret = archive_write_set_format_filter_by_ext(a, PackedFile);

	if (ret == ARCHIVE_WARN)
	{
		printf("libarchive: %s\n", archive_error_string(a));
		/*
		if (errmsg(archive_error_string(a), MB_OKCANCEL | MB_ICONWARNING) == ID_CANCEL)
		{
			archive_write_free(a);
			return 0;
		}
		*/
	}
	else if (ret < ARCHIVE_OK)
	{
		errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
		archive_write_free(a);
		return 0;
	}

	if ((options[0] != '\0') && archive_write_set_options(a, options) < ARCHIVE_OK)
	{
		errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
	}

	archive_write_set_passphrase_callback(a, NULL, archive_password_cb);

	if (Flags & PK_PACK_ENCRYPT)
		// zip: traditional, aes128, aes256
		if (archive_write_set_options(a, "encryption=traditional") < ARCHIVE_OK)
			errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);

	if (archive_write_open_filename(a, PackedFile) < ARCHIVE_OK)
		errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);

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

		while ((ret = lstat(infile, &st)) != 0)
		{
			int errsv = errno;
			asprintf(&msg, "%s: %s", infile, strerror(errsv));

			id = errmsg(msg, MB_ABORTRETRYIGNORE | MB_ICONERROR);

			if (id == ID_ABORT)
				result = E_EABORTED;

			free(msg);

			if (id != ID_RETRY)
				break;
		}

		if (ret == 0)
		{

			entry = archive_entry_new();

			if (strcmp(ext, ".mtree") == 0 && mtree_opts_nodata())
			{
				archive_entry_copy_stat(entry, &st);
				pw = getpwuid(st.st_uid);
				gr = getgrgid(st.st_gid);

				if (gr)
					archive_entry_set_gname(entry, gr->gr_name);

				if (pw)
					archive_entry_set_uname(entry, pw->pw_name);

				if (S_ISLNK(st.st_mode))
				{
					if ((len = readlink(pkfile, link, sizeof(link) - 1)) != -1)
					{
						link[len] = '\0';
						archive_entry_set_symlink(entry, link);
					}
					else
						archive_entry_set_symlink(entry, "");
				}

				if ((options[0] == '\0') || (strstr(options, "!flags") == NULL))
				{
					if (S_ISFIFO(st.st_mode))
					{
						asprintf(&msg, "%s: ignoring flags for named pipe, deal with it.", infile);
						if (errmsg(msg, MB_OKCANCEL | MB_ICONWARNING) == ID_CANCEL)
							result = E_EABORTED;
						free(msg);
					}
					else
					{
						int stflags;

						if ((fd = open(infile, O_RDONLY)) == -1)
						{
							int errsv = errno;
							//printf("libarchive: %s: %s\n", infile, strerror(errsv));
							asprintf(&msg, "%s: %s", infile, strerror(errsv));
							if (errmsg(msg, MB_OKCANCEL | MB_ICONWARNING) == ID_CANCEL)
								result = E_EABORTED;
							free(msg);
						}

						if (fd != -1)
						{
							ret = ioctl(fd, FS_IOC_GETFLAGS, &stflags);

							if (ret == 0 && stflags != 0)
								archive_entry_set_fflags(entry, stflags, 0);
						}

						close(fd);
					}
				}

				archive_entry_set_pathname(entry, pkfile);
				archive_write_header(a, entry);

				if (gProcessDataProc(infile, st.st_size) == 0)
					result = E_EABORTED;
			}
			else if (S_ISFIFO(st.st_mode))
			{
				asprintf(&msg, "%s: ignoring named pipe, deal with it", infile);
				if (errmsg(msg, MB_OKCANCEL | MB_ICONWARNING) == ID_CANCEL)
					result = E_EABORTED;
				free(msg);
			}
			else
			{

				while ((fd = open(infile, O_RDONLY)) == -1)
				{
					int errsv = errno;
					asprintf(&msg, "%s: %s", infile, strerror(errsv));

					id = errmsg(msg, MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_DEFBUTTON3);

					if (id == ID_ABORT)
						result = E_EABORTED;

					free(msg);

					if (id != ID_RETRY)
						break;
				}

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
							errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
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
			}
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
