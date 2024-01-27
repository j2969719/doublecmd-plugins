#define _GNU_SOURCE
#include <stdlib.h>
#include <glib.h>
#include <gcrypt.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

#define BUFF_SIZE 4096

typedef struct sArcData
{
	char dir_path[PATH_MAX];
	char last_path[PATH_MAX];
	int algo;
	int last_file_len;
	gboolean op_test;
	size_t same;
	size_t differ;
	size_t na;
	GRegex *re;
	FILE *fp;
	gchar *last_hash;
	tChangeVolProc ChangeVolProc;
	tProcessDataProc ProcessDataProc;
} tArcData;

typedef tArcData* ArcData;
typedef void *HINSTANCE;
tChangeVolProc gChangeVolProc = NULL;
tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gStartupInfo = NULL;

static char gLastPath[PATH_MAX];
static int gFileNamePos;
static const char *gDialogData = R"(
object DialogBox: TDialogBox
  Left = 245
  Height = 105
  Top = 158
  Width = 518
  AutoSize = True
  BorderStyle = bsDialog
  Caption = 'Double Commander'
  ChildSizing.LeftRightSpacing = 10
  ChildSizing.TopBottomSpacing = 10
  ClientHeight = 105
  ClientWidth = 518
  OnCreate = DialogBoxShow
  Position = poOwnerFormCenter
  LCLVersion = '2.0.7.0'
  object lblMsg: TLabel
    AnchorSideLeft.Control = Owner
    AnchorSideTop.Control = Owner
    AnchorSideRight.Control = fneNewFile
    AnchorSideRight.Side = asrBottom
    Left = 10
    Height = 1
    Top = 10
    Width = 500
    Anchors = [akTop, akLeft, akRight]
    ParentColor = False
    WordWrap = True
  end
  object fneNewFile: TFileNameEdit
    AnchorSideLeft.Control = Owner
    AnchorSideTop.Control = lblMsg
    AnchorSideTop.Side = asrBottom
    Left = 10
    Height = 24
    Top = 21
    Width = 500
    FilterIndex = 0
    HideDirectories = False
    ButtonWidth = 23
    NumGlyphs = 1
    BorderSpacing.Top = 10
    MaxLength = 0
    TabOrder = 2
  end
  object btnOK: TBitBtn
    AnchorSideTop.Control = fneNewFile
    AnchorSideTop.Side = asrBottom
    AnchorSideRight.Control = fneNewFile
    AnchorSideRight.Side = asrBottom
    AnchorSideBottom.Control = Owner
    AnchorSideBottom.Side = asrBottom
    Left = 410
    Height = 30
    Top = 65
    Width = 100
    Anchors = [akTop, akRight]
    AutoSize = True
    BorderSpacing.Top = 20
    Constraints.MinWidth = 100
    Default = True
    DefaultCaption = True
    Kind = bkOK
    ModalResult = 1
    OnClick = ButtonClick
    TabOrder = 0
  end
  object btnCancel: TBitBtn
    AnchorSideTop.Control = btnOK
    AnchorSideRight.Control = btnOK
    Left = 300
    Height = 30
    Top = 65
    Width = 100
    Anchors = [akTop, akRight]
    AutoSize = True
    BorderSpacing.Right = 10
    Cancel = True
    Constraints.MinWidth = 100
    DefaultCaption = True
    Kind = bkCancel
    ModalResult = 2
    OnClick = ButtonClick
    TabOrder = 1
  end
end
)";

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

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		gStartupInfo->SendDlgMsg(pDlg, "fneNewFile", DM_SETTEXT, (intptr_t)gLastPath, 0);
		char *msg;
		asprintf(&msg, "Failed to access file \"%s\", please specify a new path to the file.", gLastPath + gFileNamePos);
		gStartupInfo->SendDlgMsg(pDlg, "lblMsg", DM_SETTEXT, (intptr_t)msg, 0);
		free(msg);

		break;
	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
			g_strlcpy(gLastPath, (char*)gStartupInfo->SendDlgMsg(pDlg, "fneNewFile", DM_GETTEXT, 0, 0), PATH_MAX);

		break;
	}

	return 0;
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

	else if (strcasecmp(ext, ".blake2b") == 0)
		ret = 318 /* GCRY_MD_BLAKE2B_512 */;
	else if (strcasecmp(ext, ".blake2s") == 0)
		ret = 322 /* GCRY_MD_BLAKE2S_256 */;

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
//	else if (strcasecmp(ext, ".shake128") == 0)
//		ret = 316 /* GCRY_MD_SHAKE128 */;
//	else if (strcasecmp(ext, ".shake256") == 0)
//		ret = 317 /* GCRY_MD_SHAKE256 */;
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
	else if (strcasecmp(ext, ".sm3") == 0)
		ret = 326 /* GCRY_MD_SM3 */;
	else if (strcasecmp(ext, ".sha512_256") == 0)
		ret = 327 /* GCRY_MD_SHA512_256 */;
	else if (strcasecmp(ext, ".sha512_224") == 0)
		ret = 328 /* GCRY_MD_SHA512_224 */;

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
	struct stat buf;
	size_t csize = 0, prcnt;

	if (lstat(path, &buf) != 0)
		return NULL;

	if (S_ISLNK(buf.st_mode))
	{
		if (stat(path, &buf) != 0 || !S_ISREG(buf.st_mode))
			return NULL;

		char *msg;
		asprintf(&msg, "\"%s\" is not a regular file.", path);
		errmsg(msg, MB_OK | MB_ICONWARNING);
		free(msg);
	}
	else if (!S_ISREG(buf.st_mode))
		return NULL;

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

		if (buf.st_size > 0)
		{
			csize += bytes;
			prcnt = csize * 100 / buf.st_size;
		}
		else
			prcnt = 0;

		proc(path, -(1000 + prcnt));
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
	char arc[PATH_MAX];
	GError *err = NULL;
	tArcData * handle;

	handle = malloc(sizeof(tArcData));

	if (handle == NULL)
	{
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	memset(handle, 0, sizeof(tArcData));

	g_strlcpy(arc, ArchiveData->ArcName, PATH_MAX);

	const char *ext = strrchr(arc, '.');
	handle->algo = get_algo_from_ext(ext);

	if (handle->algo == -1)
	{
		free(handle);
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

	handle->re = g_regex_new("^([a-f0-9]{6,})\\s+\\*?([^\\n]+)$", G_REGEX_CASELESS, 0, &err);

	if (handle->re == NULL)
	{
		free(handle);

		if (err)
		{
			errmsg((err)->message, MB_OK | MB_ICONERROR);
			g_error_free(err);
		}

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

	g_strlcpy(handle->dir_path, dirname(arc), PATH_MAX);
	handle->op_test = FALSE;

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
	GMatchInfo *match_info;

	memset(HeaderDataEx, 0, sizeof(&HeaderDataEx));
	ArcData handle = (ArcData)hArcData;

	if (handle->last_hash != NULL)
	{
		g_free(handle->last_hash);
		handle->last_hash = NULL;
	}


	if ((lread = getline(&line, &len, handle->fp)) != -1)
	{
		if (!g_regex_match(handle->re, line, 0, &match_info))
		{
			errmsg("Failed to parse file.", MB_OK | MB_ICONERROR);
			return E_BAD_ARCHIVE;
		}

		handle->last_hash = g_strdup(g_match_info_fetch(match_info, 1));
		gchar *str_file = g_match_info_fetch(match_info, 2);

		if (str_file[0] != '/')
		{
			g_strlcpy(HeaderDataEx->FileName, str_file, sizeof(HeaderDataEx->FileName) - 1);
			snprintf(handle->last_path, PATH_MAX, "%s/%s", handle->dir_path, str_file);
		}
		else
		{
			g_strlcpy(HeaderDataEx->FileName, str_file + 1, sizeof(HeaderDataEx->FileName) - 1);
			g_strlcpy(handle->last_path, str_file, PATH_MAX);
		}

		handle->last_file_len = strlen(HeaderDataEx->FileName);

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

	if (match_info)
		g_match_info_free(match_info);

	if (line)
		free(line);

	return E_SUCCESS;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	ArcData handle = (ArcData)hArcData;

	if (Operation != PK_SKIP && access(handle->last_path, F_OK) == -1)
	{
		g_strlcpy(gLastPath, handle->last_path, PATH_MAX);
		gFileNamePos = strlen(gLastPath) - handle->last_file_len;

		if (gStartupInfo == NULL && handle->ChangeVolProc(gLastPath, PK_VOL_ASK) == 0)
			return E_EABORTED;
		else if (gStartupInfo->DialogBoxLFM((intptr_t)gDialogData, strlen(gDialogData), DlgProc) == 0)
			return E_EABORTED;

		g_strlcpy(handle->last_path, gLastPath, PATH_MAX);

		int len = strlen(gLastPath);

		if (len > handle->last_file_len)
			g_strlcpy(handle->dir_path, gLastPath, len - handle->last_file_len);
	}

	if (Operation == PK_TEST)
	{
		if (handle->op_test == FALSE)
			handle->op_test = TRUE;

		char *hash = calc_hash(handle->algo, handle->last_path, handle->ProcessDataProc);

		if (hash)
		{
			if (strcasecmp(hash, handle->last_hash) != 0)
			{
				char *msg;
				asprintf(&msg, "%s: different hash value (%s).", handle->last_path, gcry_md_algo_name(handle->algo));
				int ret = errmsg(msg, MB_OKCANCEL | MB_ICONERROR);
				free(msg);
				handle->differ++;

				if (ret != ID_OK)
					return E_EABORTED;
			}
			else
				handle->same++;

			free(hash);
		}
		else if (handle->ProcessDataProc && handle->ProcessDataProc(handle->last_path, -1100) == 0)
			return E_EABORTED;
		else if (gcry_md_test_algo(handle->algo) != 0)
			return E_NOT_SUPPORTED;
		else
		{
			char *msg;
			asprintf(&msg, "Unable to compute hash (%s) of \"%s\".", gcry_md_algo_name(handle->algo), handle->last_path);
			int ret = errmsg(msg, MB_OKCANCEL | MB_ICONERROR);
			free(msg);
			handle->na++;

			if (ret != ID_OK)
				return E_EABORTED;
		}

	}
	else if (Operation == PK_EXTRACT && !DestPath)
	{
		char *msg;
		asprintf(&msg, "%s: %s\n\nCreate a symlink to %s?", gcry_md_algo_name(handle->algo), handle->last_hash, handle->last_path);
		int ret = errmsg(msg, MB_YESNO | MB_ICONQUESTION);
		free(msg);

		if (ret != ID_YES)
			return E_EABORTED;

		if (symlink(handle->last_path, DestName) != 0)
			return E_EWRITE;
	}

	return E_SUCCESS;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData handle = (ArcData)hArcData;

	if (handle->last_hash != NULL)
		g_free(handle->last_hash);

	fclose(handle->fp);
	g_regex_unref(handle->re);

	if (handle->op_test == TRUE)
	{
		char *msg;
		asprintf(&msg, "Result:\n\n\ttotal files: %ld\n\tsame: %ld\n\tdifferent %ld\n\tunable to compute hash: %ld",
				handle->same+handle->differ+handle->na, handle->same, handle->differ, handle->na);
		errmsg(msg, MB_OK | MB_ICONINFORMATION);
		free(msg);
	}

	free(handle);
	return E_SUCCESS;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData handle = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !handle)
		gProcessDataProc = pProcessDataProc;
	else
		handle->ProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{
	ArcData handle = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !handle)
		gChangeVolProc = pChangeVolProc1;
	else
		handle->ChangeVolProc = pChangeVolProc1;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	const char *ext = strrchr(FileName, '.');

	if (get_algo_from_ext(ext) == -1)
		return FALSE;
	else
		return TRUE;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_HIDE;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	FILE *fp;
	char path[PATH_MAX];
	const char *ext = strrchr(PackedFile, '.');
	int algo = get_algo_from_ext(ext);

	if (gcry_md_test_algo(algo) != 0)
		return E_NOT_SUPPORTED;

	if (access(PackedFile, F_OK) != -1)
		errmsg("Only append to file is supported.", MB_OK | MB_ICONWARNING);

	if ((fp = fopen(PackedFile, "a")) == NULL)
		return E_EWRITE;

	while (*AddList)
	{
		if (AddList[strlen(AddList) - 1] != '/')
		{
			snprintf(path, PATH_MAX, "%s%s", SrcPath, AddList);

			char *hash = calc_hash(algo, path, gProcessDataProc);

			if (hash)
			{
				fprintf(fp, "%s *%s\n", hash, AddList);
				free(hash);
			}
			else if (gProcessDataProc(path, 0) == 0)
				break;
			else
			{
				char *msg;
				asprintf(&msg, "Unable to compute hash (%s) of \"%s\".", gcry_md_algo_name(algo), path);

				if (errmsg(msg, MB_OKCANCEL | MB_ICONERROR) != ID_OK)
					break;

				free(msg);
			}
		}

		while (*AddList++);
	}

	fclose(fp);

	return E_SUCCESS;
}
