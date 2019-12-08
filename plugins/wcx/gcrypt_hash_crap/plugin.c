#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pcre.h>
#include <gcrypt.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

#define BUFF_SIZE 1024
#define OVECCOUNT 30

typedef struct sArcData
{
	char dir_path[PATH_MAX];
	char last_path[PATH_MAX];
	int algo;
	pcre *re;
	FILE *fp;
	char *last_hash;
	tChangeVolProc gChangeVolProc;
	tProcessDataProc gProcessDataProc;
} tArcData;

typedef tArcData* ArcData;
typedef void *HINSTANCE;
tChangeVolProc gChangeVolProc = NULL;
tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gStartupInfo = NULL;

char* strlcpy(char* p, const char* p2, int maxlen)
{
	if ((int)strlen(p2) >= maxlen)
	{
		strncpy(p, p2, maxlen);
		p[maxlen] = 0;
	}
	else
		strcpy(p, p2);

	return p;
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gStartupInfo == NULL)
	{
		gStartupInfo = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gStartupInfo, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gStartupInfo != NULL)
		free(gStartupInfo);

	gStartupInfo = NULL;
}

static int errmsg(const char *msg, long flags)
{
	if (gStartupInfo)
		return gStartupInfo->MessageBox(msg ? (char*)msg : "Unknown error", NULL, flags);
	else
	{
		printf("(gStartupInfo->MessageBox, gStartupInfo == NULL): %s", msg);
		return ID_ABORT;
	}
}

static int get_algo_from_ext(const char*ext)
{
	int ret;

	if (ext == NULL)
		ret = -1;
	else if (strcasecmp(ext, ".md5") == 0)
		ret = 1 /* GCRY_MD_MD5 */;
	else if (strcasecmp(ext, ".sha1") == 0)
		ret = 2 /* GCRY_MD_SHA1 */;
	else if (strcasecmp(ext, ".rmd160") == 0)
		ret = 3 /* GCRY_MD_RMD160 */;
	else if (strcasecmp(ext, ".md2") == 0)
		ret = 5 /* GCRY_MD_MD2 */;
	else if (strcasecmp(ext, ".tiger") == 0)
		ret = 6 /* GCRY_MD_TIGER */;
	else if (strcasecmp(ext, ".haval") == 0)
		ret = 7 /* GCRY_MD_HAVAL */;
	else if (strcasecmp(ext, ".sha256") == 0)
		ret = 8 /* GCRY_MD_SHA256 */;
	else if (strcasecmp(ext, ".sha384") == 0)
		ret = 9 /* GCRY_MD_SHA384 */;
	else if (strcasecmp(ext, ".sha512") == 0)
		ret = 10 /* GCRY_MD_SHA512 */;
	else if (strcasecmp(ext, ".sha224") == 0)
		ret = 11 /* GCRY_MD_SHA224 */;
	else if (strcasecmp(ext, ".md4") == 0)
		ret = 301 /* GCRY_MD_MD4 */;
	else if (strcasecmp(ext, ".crc32") == 0)
		ret = 302 /* GCRY_MD_CRC32 */;
	else if (strcasecmp(ext, ".crc32_rfc1510") == 0)
		ret = 303 /* GCRY_MD_CRC32_RFC1510 */;
	else if (strcasecmp(ext, ".crc24_rfc2440") == 0)
		ret = 304 /* GCRY_MD_CRC24_RFC2440 */;
	else if (strcasecmp(ext, ".whirlpool") == 0)
		ret = 305 /* GCRY_MD_WHIRLPOOL */;
	else if (strcasecmp(ext, ".tiger1") == 0)
		ret = 306 /* GCRY_MD_TIGER1 */;
	else if (strcasecmp(ext, ".tiger2") == 0)
		ret = 307 /* GCRY_MD_TIGER2 */;
	else if (strcasecmp(ext, ".gostr3411_94") == 0)
		ret = 308 /* GCRY_MD_GOSTR3411_94 */;
	else if (strcasecmp(ext, ".stribog256") == 0)
		ret = 309 /* GCRY_MD_STRIBOG256 */;
	else if (strcasecmp(ext, ".stribog512") == 0)
		ret = 310 /* GCRY_MD_STRIBOG512 */;
	else if (strcasecmp(ext, ".gostr3411_cp") == 0)
		ret = 311 /* GCRY_MD_GOSTR3411_CP */;
	else if (strcasecmp(ext, ".sha3_224") == 0)
		ret = 312 /* GCRY_MD_SHA3_224 */;
	else if (strcasecmp(ext, ".sha3_256") == 0)
		ret = 313 /* GCRY_MD_SHA3_256 */;
	else if (strcasecmp(ext, ".sha3_384") == 0)
		ret = 314 /* GCRY_MD_SHA3_384 */;
	else if (strcasecmp(ext, ".sha3_512") == 0)
		ret = 315 /* GCRY_MD_SHA3_512 */;
	else if (strcasecmp(ext, ".blake2b_512") == 0)
		ret = 318 /* GCRY_MD_BLAKE2B_512 */;
	else if (strcasecmp(ext, ".blake2b_384") == 0)
		ret = 319 /* GCRY_MD_BLAKE2B_384 */;
	else if (strcasecmp(ext, ".blake2b_256") == 0)
		ret = 320 /* GCRY_MD_BLAKE2B_256 */;
	else if (strcasecmp(ext, ".blake2b_160") == 0)
		ret = 321 /* GCRY_MD_BLAKE2B_160 */;
	else if (strcasecmp(ext, ".blake2s_256") == 0)
		ret = 322 /* GCRY_MD_BLAKE2S_256 */;
	else if (strcasecmp(ext, ".blake2s_224") == 0)
		ret = 323 /* GCRY_MD_BLAKE2S_224 */;
	else if (strcasecmp(ext, ".blake2s_160") == 0)
		ret = 324 /* GCRY_MD_BLAKE2S_160 */;
	else if (strcasecmp(ext, ".blake2s_128") == 0)
		ret = 325 /* GCRY_MD_BLAKE2S_128 */;
	else
		ret = -1;

	return ret;
}

static char* calc_hash(int algo, char *path, tProcessDataProc proc)
{
	gcry_md_hd_t h;
	gcry_error_t err;
	size_t i, bytes, res_size;
	unsigned char data[BUFF_SIZE];
	char* result = NULL;
	unsigned char* digest;
	unsigned int digestlen;

	FILE *inFile = fopen(path, "rb");

	if (inFile == NULL)
		return NULL;

	err = gcry_md_open(&h, algo, 0);

	if (gcry_err_code(err))
	{
		fclose(inFile);
		return NULL;
	}

	while ((bytes = fread(data, 1, BUFF_SIZE, inFile)) != 0)
	{
		if (proc && proc(path, bytes) == 0)
		{
			fclose(inFile);
			gcry_md_close(h);
			return NULL;
		}

		gcry_md_write(h, data, bytes);
	}

	digest = gcry_md_read(h, algo);
	digestlen = gcry_md_get_algo_dlen(algo);

	if (digestlen > 0)
	{
		res_size = (size_t)digestlen * sizeof(char) * 2 + 1;
		result = (char*)malloc(res_size);
	}

	if (result != NULL)
	{
		memset(result, 0, res_size);

		for (i = 0; i < digestlen; i++)
			sprintf(&result[i * 2], "%02x", digest[i]);
	}

	fclose(inFile);
	gcry_md_close(h);
	return result;

}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	const char *err;
	char arc[PATH_MAX];
	int erroffset;
	tArcData * handle;

	handle = malloc(sizeof(tArcData));

	if (handle == NULL)
	{
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	memset(handle, 0, sizeof(tArcData));

	strlcpy(arc, ArchiveData->ArcName, PATH_MAX);

	const char *ext = strrchr(arc, '.');
	handle->algo = get_algo_from_ext(ext);

	if (handle->algo == -1)
	{
		free(handle);
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

	handle->re = pcre_compile("^([a-f0-9]+)\\s\\*([^\\n]+)$", 0, &err, &erroffset, NULL);

	if (handle->re == NULL)
	{
		free(handle);
		errmsg(err, MB_OK | MB_ICONERROR);
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

	handle->fp = fopen(arc, "r");

	if (handle->fp == NULL)
	{
		free(handle);
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

	strlcpy(handle->dir_path, dirname(arc), PATH_MAX);

	return (HANDLE)handle;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t lread;
	struct stat buf;
	int rc;
	int ovector[OVECCOUNT];

	memset(HeaderDataEx, 0, sizeof(&HeaderDataEx));
	ArcData handle = (ArcData)hArcData;

	if (handle->last_hash != NULL)
	{
		free(handle->last_hash);
		handle->last_hash = NULL;
	}


	if ((lread = getline(&line, &len, handle->fp)) != -1)
	{
		rc = pcre_exec(handle->re, NULL, line, lread, 0, 0, ovector, OVECCOUNT);

		if (rc < 1)
		{
			errmsg("Failed to parse file.", MB_OK | MB_ICONERROR);
			return E_BAD_ARCHIVE;
		}

		char *start = line + ovector[2];
		int length = ovector[3] - ovector[2];
		asprintf(&handle->last_hash, "%.*s", length, start);

		start = line + ovector[4];
		length = ovector[5] - ovector[4];

		if (length < sizeof(HeaderDataEx->FileName))
			strlcpy(HeaderDataEx->FileName, start, length);
		else
			strlcpy(HeaderDataEx->FileName, start, sizeof(HeaderDataEx->FileName) - 1);

		if (start[0] != '/')
			snprintf(handle->last_path, PATH_MAX, "%s/%s", handle->dir_path, HeaderDataEx->FileName);
		else
			strlcpy(handle->last_path, HeaderDataEx->FileName, PATH_MAX);

		if (lstat(handle->last_path, &buf) == 0)
		{
			HeaderDataEx->PackSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
			HeaderDataEx->PackSize = buf.st_size & 0x00000000FFFFFFFF;
			HeaderDataEx->UnpSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
			HeaderDataEx->UnpSize = buf.st_size & 0x00000000FFFFFFFF;
			HeaderDataEx->FileTime = buf.st_mtime;
			HeaderDataEx->FileAttr = buf.st_mode;
		}
	}
	else
		return E_END_ARCHIVE;

	if (line)
		free(line);

	return E_SUCCESS;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	ArcData handle = (ArcData)hArcData;

	if (Operation == PK_TEST)
	{
		char *hash = calc_hash(handle->algo, handle->last_path, handle->gProcessDataProc);

		if (hash)
		{
			if (strcmp(hash, handle->last_hash) != 0)
				return E_BAD_DATA;

			free(hash);
		}
		else
			return E_NOT_SUPPORTED;
	}
	else if (Operation == PK_EXTRACT && !DestPath)
	{
		if (symlink(handle->last_path, DestName) != 0)
			return E_EWRITE;
	}

	return E_SUCCESS;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData handle = (ArcData)hArcData;

	if (handle->last_hash != NULL)
		free(handle->last_hash);

	fclose(handle->fp);
	pcre_free(handle->re);
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

}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	const char *ext = strrchr(FileName, '.');

	if (get_algo_from_ext(ext) == -1)
		return false;
	else
		return true;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_BY_CONTENT;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	FILE *fp;
	char path[PATH_MAX];
	const char *ext = strrchr(PackedFile, '.');
	int algo = get_algo_from_ext(ext);

	if ((fp = fopen(PackedFile, "a")) == NULL)
		return E_EWRITE;

	while (*AddList)
	{
		snprintf(path, PATH_MAX, "%s%s", SrcPath, AddList);

		char *hash = calc_hash(algo, path, gProcessDataProc);

		if (hash)
		{
			fprintf(fp, "%s *%s\n", hash, AddList);
			free(hash);
		}
		else
		{
			fclose(fp);
			return E_NOT_SUPPORTED;
		}

		while (*AddList++);
	}

	fclose(fp);

	return E_SUCCESS;
}
