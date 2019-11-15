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
#include <ftw.h>
#include <fnmatch.h>
#include <dlfcn.h>
#include <limits.h>

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

#define BUFF_SIZE 8192

static unsigned char lizard_magic[] = { 0x06, 0x22, 0x4D, 0x18 };

tChangeVolProc gChangeVolProc  = NULL;
tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gStartupInfo = NULL;
static char gOptions[PATH_MAX];
static char gEncryption[PATH_MAX] = "encryption=traditional";
static char gLFMPath[PATH_MAX];
static bool gMtreeClasic = false;
static bool gMtreeCheckFS = false;
static bool gReadCompat2x = false;
static bool gReadIgnoreCRC = false;
static bool gReadMacExt = false;
static bool gReadDisableJoliet = false;
static bool gReadDisableRockridge = false;
static bool gExtension = false;
static char gReadCharset[256];
static int  gTarFormat = ARCHIVE_FORMAT_TAR;


void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	printf("----> ExtensionInitialize\n");
	Dl_info dlinfo;
	const char* lfm_name = "dialog.lfm";

	gStartupInfo = malloc(sizeof(tExtensionStartupInfo));
	memcpy(gStartupInfo, StartupInfo, sizeof(tExtensionStartupInfo));

	gExtension = true;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(lfm_name, &dlinfo) != 0)
	{
		strncpy(gLFMPath, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(gLFMPath, '/');

		if (pos)
			strcpy(pos + 1, lfm_name);
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	printf("----> ExtensionFinalize\n");
	if (gExtension && gStartupInfo != NULL)
		free(gStartupInfo);

	gExtension = false;
}

static int errmsg(const char *msg, long flags)
{
	return gStartupInfo->MessageBox(msg ? (char*)msg : "Unknown error", NULL, flags);
}

static void remove_file(const char *file)
{
	if (remove(file) == -1)
	{
		int errsv = errno;
		printf("remove file: %s: %s\n", file, strerror(errsv));
	}
}

static int nftw_remove_cb(const char *file, const struct stat *bif, int tflag, struct FTW *ftwbuf)
{
	remove_file(file);
	return 0;
}

static void remove_target(const char *filename)
{
	struct stat st;

	if (lstat(filename, &st) == 0)
	{
		if S_ISDIR(st.st_mode)
			nftw(filename, nftw_remove_cb, 13, FTW_DEPTH | FTW_PHYS);
		else
			remove_file(filename);
	}
}

static bool mtree_opts_nodata(void)
{
	bool result = true;

	if (gOptions[0] != '\0')
	{
		if (strstr(gOptions, "dironly") != NULL)
			result = true;
		else if (strstr(gOptions, "all") != NULL)
			result = false;
		else if (strstr(gOptions, "cksum") != NULL)
			result = false;
		else if (strstr(gOptions, "md5") != NULL)
			result = false;
		else if (strstr(gOptions, "rmd160") != NULL)
			result = false;
		else if (strstr(gOptions, "sha1") != NULL)
			result = false;
		else if (strstr(gOptions, "sha256") != NULL)
			result = false;
		else if (strstr(gOptions, "sha384") != NULL)
			result = false;
		else if (strstr(gOptions, "sha512") != NULL)
			result = false;
	}

	return result;
}

static int archive_set_format_filter(struct archive *a, const char*ext)
{
	int ret;

	if (strcasecmp(ext, ".tzst") == 0)
	{
		ret = archive_write_set_format(a, gTarFormat);
		ret = archive_write_add_filter_zstd(a);
	}
	else if (strcasecmp(ext, ".zst") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_zstd(a);
	}
	else if (strcasecmp(ext, ".mtree") == 0)
	{
		if (gMtreeClasic)
			ret = archive_write_set_format_mtree_classic(a);
		else
			ret = archive_write_set_format_mtree(a);
	}
	else if (strcasecmp(ext, ".lz4") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lz4(a);
	}
	else if (strcasecmp(ext, ".lz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lzip(a);
	}
	else if (strcasecmp(ext, ".lzo") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lzop(a);
	}
	else if (strcasecmp(ext, ".lrz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lrzip(a);
	}
	else if (strcasecmp(ext, ".grz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_grzip(a);
	}
	else if (strcasecmp(ext, ".lzma") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_lzma(a);
	}
	else if (strcasecmp(ext, ".b64") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_b64encode(a);
	}
	else if (strcasecmp(ext, ".uue") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_uuencode(a);
	}
	else if (strcasecmp(ext, ".gz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_gzip(a);
	}
	else if (strcasecmp(ext, ".xz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_xz(a);
	}
	else if (strcasecmp(ext, ".z") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_compress(a);
	}
	else if (strcasecmp(ext, ".bz2") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_bzip2(a);
	}
	else if (strcasecmp(ext, ".warc") == 0)
	{
		ret = archive_write_set_format_warc(a);
	}
	else if (strcasecmp(ext, ".xar") == 0)
	{
		ret = archive_write_set_format_xar(a);
	}
	else if (strcasecmp(ext, ".tar") == 0)
	{
		ret = archive_write_set_format(a, gTarFormat);
	}
	else if (strcasecmp(ext, ".tgz") == 0)
	{
		ret = archive_write_set_format(a, gTarFormat);
		ret = archive_write_add_filter_gzip(a);
	}
	else if (strcasecmp(ext, ".tbz2") == 0)
	{
		ret = archive_write_set_format(a, gTarFormat);
		ret = archive_write_add_filter_bzip2(a);
	}
	else if (strcasecmp(ext, ".txz") == 0)
	{
		ret = archive_write_set_format(a, gTarFormat);
		ret = archive_write_add_filter_xz(a);
	}
	else if (strcasecmp(ext, ".liz") == 0)
	{
		ret = archive_write_set_format_raw(a);
		ret = archive_write_add_filter_program(a, "lizard");
	}
	else
		ret = archive_write_set_format_filter_by_ext(a, ext);

	return ret;
}

const char *archive_password_cb(struct archive *a, void *data)
{
	static char pass[PATH_MAX];

	if (gStartupInfo->InputBox("Double Commander", "Please enter the password:", true, pass, PATH_MAX - 1))
		return pass;
	else
		return NULL;
}

static void checkbox_get_option(uintptr_t pDlg, char* DlgItemName, const char* optstr, bool defval, char *strval)
{
	bool chk = (bool)gStartupInfo->SendDlgMsg(pDlg, DlgItemName, DM_GETCHECK, 0, 0);

	if ((chk && !defval) || (!chk && defval))
	{
		if (strval[strlen(strval) - 1] != ':')
			strcat(strval, ",");

		if (!chk && defval)
			strcat(strval, "!");

		strcat(strval, optstr);
	}
}

static void textfield_get_option(uintptr_t pDlg, char* DlgItemName, const char* optstr, char *strval)
{
	char *tmpval = malloc(PATH_MAX);
	memset(tmpval, 0, PATH_MAX);
	strncpy(tmpval, (char*)gStartupInfo->SendDlgMsg(pDlg, DlgItemName, DM_GETTEXT, 0, 0), PATH_MAX);

	if (tmpval[0] != '\0')
	{
		if (strval[strlen(strval) - 1] != ':')
			strcat(strval, ",");

		snprintf(strval, PATH_MAX, "%s%s=%s", strdup(strval), optstr, tmpval);
	}

	free(tmpval);
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	int numval;
	bool bval;
	char *strval = malloc(PATH_MAX);
	memset(strval, 0, PATH_MAX);

	switch (Msg)
	{
	case DN_INITDIALOG:
		gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, (intptr_t)gOptions, 0);
		gStartupInfo->SendDlgMsg(pDlg, "chkClassic", DM_SETCHECK, (intptr_t)gMtreeClasic, 0);
		snprintf(strval, PATH_MAX, "%s", archive_version_details());
		gStartupInfo->SendDlgMsg(pDlg, "lblInfo", DM_SETTEXT, (intptr_t)strval, 0);
		gStartupInfo->SendDlgMsg(pDlg, "cbZISOCompLvl", DM_ENABLE, 0, 0);

		switch (gTarFormat)
		{
		case ARCHIVE_FORMAT_TAR_USTAR:
			numval = 1;
			break;

		case ARCHIVE_FORMAT_TAR_PAX_INTERCHANGE:
			numval = 2;
			break;

		case ARCHIVE_FORMAT_TAR_PAX_RESTRICTED:
			numval = 3;
			break;

		case ARCHIVE_FORMAT_TAR_GNUTAR:
			numval = 4;
			break;

		default:
			numval = 0;
		}

		gStartupInfo->SendDlgMsg(pDlg, "cbTarFormat", DM_LISTSETITEMINDEX, numval, 0);

		gStartupInfo->SendDlgMsg(pDlg, "cbReadCharset", DM_SETTEXT, (intptr_t)gReadCharset, 0);
		gStartupInfo->SendDlgMsg(pDlg, "chkReadCompat2x", DM_SETCHECK, (intptr_t)gReadCompat2x, 0);
		gStartupInfo->SendDlgMsg(pDlg, "chkReadMtreeckfs", DM_SETCHECK, (intptr_t)gMtreeCheckFS, 0);
		gStartupInfo->SendDlgMsg(pDlg, "chkReadMacExt", DM_SETCHECK, (intptr_t)gReadMacExt, 0);
		gStartupInfo->SendDlgMsg(pDlg, "chkReadJoliet", DM_SETCHECK, (intptr_t)gReadDisableJoliet, 0);
		gStartupInfo->SendDlgMsg(pDlg, "chkReadRockridge", DM_SETCHECK, (intptr_t)gReadDisableRockridge, 0);
		gStartupInfo->SendDlgMsg(pDlg, "chkReadIgnoreCRC", DM_SETCHECK, (intptr_t)gReadIgnoreCRC, 0);

		break;

	case DN_CLICK:
		if (strncmp(DlgItemName, "btnOK", 5) == 0)
		{
			strncpy(gOptions, (char*)gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_GETTEXT, 0, 0), PATH_MAX);
			gMtreeClasic = (bool)gStartupInfo->SendDlgMsg(pDlg, "chkClassic", DM_GETCHECK, 0, 0);
			snprintf(gEncryption, PATH_MAX, "encryption=%s", (char*)gStartupInfo->SendDlgMsg(pDlg, "cbEncrypt", DM_GETTEXT, 0, 0));

			gStartupInfo->SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, 1, 0);
		}
		else if (strncmp(DlgItemName, "btnClear", 8) == 0)
			gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, 0, 0);

		break;

	case DN_CHANGE:
		if (strncmp(DlgItemName, "chkMtree", 8) == 0)
		{
			bval = (bool)gStartupInfo->SendDlgMsg(pDlg, "chkMtreeAll", DM_GETCHECK, 0, 0);
			gStartupInfo->SendDlgMsg(pDlg, "gbMtreeAttr", DM_ENABLE, (intptr_t)!bval, 0);
			gStartupInfo->SendDlgMsg(pDlg, "gbMtreeCkSum", DM_ENABLE, (intptr_t)!bval, 0);

			strncpy(strval, "mtree:", PATH_MAX);

			if (bval)
				checkbox_get_option(pDlg, "chkMtreeAll", "all", false, strval);
			else
			{
				checkbox_get_option(pDlg, "chkMtreeCksum", "cksum", false, strval);
				checkbox_get_option(pDlg, "chkMtreeDevice", "device", true, strval);
				checkbox_get_option(pDlg, "chkMtreeFlags", "flags", true, strval);
				checkbox_get_option(pDlg, "chkMtreeGid", "gid", true, strval);
				checkbox_get_option(pDlg, "chkMtreeGname", "gname", true, strval);
				checkbox_get_option(pDlg, "chkMtreeLink", "link", true, strval);
				checkbox_get_option(pDlg, "chkMtreeMd5", "md5", false, strval);
				checkbox_get_option(pDlg, "chkMtreeMode", "mode", true, strval);
				checkbox_get_option(pDlg, "chkMtreeNlink", "nlink", true, strval);
				checkbox_get_option(pDlg, "chkMtreeRmd160", "rmd160", false, strval);
				checkbox_get_option(pDlg, "chkMtreeSha1", "sha1", false, strval);
				checkbox_get_option(pDlg, "chkMtreeSha256", "sha256", false, strval);
				checkbox_get_option(pDlg, "chkMtreeSha384", "sha384", false, strval);
				checkbox_get_option(pDlg, "chkMtreeSha512", "sha512", false, strval);
				checkbox_get_option(pDlg, "chkMtreeSize", "size", true, strval);
				checkbox_get_option(pDlg, "chkMtreeTime", "time", true, strval);
				checkbox_get_option(pDlg, "chkMtreeUid", "uid", true, strval);
				checkbox_get_option(pDlg, "chkMtreeUname", "uname", true, strval);
				checkbox_get_option(pDlg, "chkMtreeResdevice", "resdevice", false, strval);
			}

			checkbox_get_option(pDlg, "chkMtreeDirsOnly", "dironly", false, strval);
			checkbox_get_option(pDlg, "chkMtreeUseSet", "use-set", false, strval);
			checkbox_get_option(pDlg, "chkMtreeIndent", "indent", false, strval);

			if (strcmp(strval, "mtree:") != 0)
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, (intptr_t)strval, 0);
			else
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, 0, 0);
		}
		else if (strcmp(DlgItemName, "cbReadCharset") == 0)
		{
			strncpy(gReadCharset, (char*)gStartupInfo->SendDlgMsg(pDlg, "cbReadCharset", DM_GETTEXT, 0, 0), 255);
		}
		else if (strcmp(DlgItemName, "chkReadMtreeckfs") == 0)
		{
			gMtreeCheckFS = (bool)gStartupInfo->SendDlgMsg(pDlg, "chkReadMtreeckfs", DM_GETCHECK, 0, 0);
		}
		else if (strcmp(DlgItemName, "chkReadCompat2x") == 0)
		{
			gReadCompat2x = (bool)gStartupInfo->SendDlgMsg(pDlg, "chkReadCompat2x", DM_GETCHECK, 0, 0);
		}
		else if (strcmp(DlgItemName, "chkReadIgnoreCRC") == 0)
		{
			gReadIgnoreCRC = (bool)gStartupInfo->SendDlgMsg(pDlg, "chkReadIgnoreCRC", DM_GETCHECK, 0, 0);
		}
		else if (strcmp(DlgItemName, "chkReadMacExt") == 0)
		{
			gReadMacExt = (bool)gStartupInfo->SendDlgMsg(pDlg, "chkReadMacExt", DM_GETCHECK, 0, 0);
		}
		else if (strcmp(DlgItemName, "chkReadJoliet") == 0)
		{
			gReadDisableJoliet = (bool)gStartupInfo->SendDlgMsg(pDlg, "chkReadJoliet", DM_GETCHECK, 0, 0);
		}
		else if (strcmp(DlgItemName, "chkReadRockridge") == 0)
		{
			gReadDisableRockridge = (bool)gStartupInfo->SendDlgMsg(pDlg, "chkReadRockridge", DM_GETCHECK, 0, 0);
		}
		else if (strncmp(DlgItemName, "cbTar", 5) == 0)
		{
			numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cbTarFormat", DM_LISTGETITEMINDEX, 0, 0);

			if (numval > -1)
			{
				switch (numval)
				{
				case 1:
					gTarFormat = ARCHIVE_FORMAT_TAR_USTAR;
					break;

				case 2:
					gTarFormat = ARCHIVE_FORMAT_TAR_PAX_INTERCHANGE;
					break;

				case 3:
					gTarFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
					break;

				case 4:
					gTarFormat = ARCHIVE_FORMAT_TAR_GNUTAR;
					break;

				default:
					gTarFormat = ARCHIVE_FORMAT_TAR;
				}
			}
		}
		else if (strncmp(DlgItemName, "cb7Z", 4) == 0)
		{
			numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cb7ZCompression", DM_LISTGETITEMINDEX, 0, 0);

			if (numval > -1)
			{
				snprintf(strval, PATH_MAX, "7zip:compression=%s", (char*)gStartupInfo->SendDlgMsg(pDlg, "cb7ZCompression", DM_GETTEXT, 0, 0));
			}
			else
				strncpy(strval, "7zip:", PATH_MAX);

			numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cb7ZCompLvl", DM_LISTGETITEMINDEX, 0, 0);

			if (numval > -1)
			{
				if (strval[strlen(strval) - 1] != ':')
					strcat(strval, ",");

				snprintf(strval, PATH_MAX, "%scompression-level=%d", strdup(strval), numval);
			}

			if (strcmp(strval, "7zip:") != 0)
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, (intptr_t)strval, 0);
			else
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, 0, 0);

		}
		else if (strstr(DlgItemName, "Zip") != NULL)
		{
			numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cbZipCompression", DM_LISTGETITEMINDEX, 0, 0);

			if (numval > -1)
			{
				snprintf(strval, PATH_MAX, "zip:compression=%s", (char*)gStartupInfo->SendDlgMsg(pDlg, "cbZipCompression", DM_GETTEXT, 0, 0));
			}
			else
				strncpy(strval, "zip:", PATH_MAX);

			numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cbZipCompLvl", DM_LISTGETITEMINDEX, 0, 0);

			if (numval > -1)
			{
				if (strval[strlen(strval) - 1] != ':')
					strcat(strval, ",");

				snprintf(strval, PATH_MAX, "%scompression-level=%d", strdup(strval), numval);
			}

			textfield_get_option(pDlg, "cbZipCharset", "hdrcharset", strval);
			checkbox_get_option(pDlg, "chkZipExperimental", "experimental", false, strval);
			checkbox_get_option(pDlg, "chkZipFakeCRC32", "fakecrc32", false, strval);
			checkbox_get_option(pDlg, "chkZip64", "zip64", true, strval);


			if (strcmp(strval, "zip:") != 0)
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, (intptr_t)strval, 0);
			else
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, 0, 0);
		}
		else if (strstr(DlgItemName, "ISO") != NULL)
		{
			strncpy(strval, "iso9660:", PATH_MAX);
			textfield_get_option(pDlg, "edISOVolumeID", "volume-id", strval);
			textfield_get_option(pDlg, "edISOAbstractFile", "abstract-file", strval);
			textfield_get_option(pDlg, "edISOApplicationID", "application-id", strval);
			textfield_get_option(pDlg, "edISOCopyrightFile", "copyright-file", strval);
			textfield_get_option(pDlg, "edISOPublisher", "publisher", strval);
			textfield_get_option(pDlg, "edISOBiblioFile", "biblio-file", strval);

			numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cbISOLevel", DM_LISTGETITEMINDEX, 0, 0);

			if (numval > -1)
			{
				if (strval[strlen(strval) - 1] != ':')
					strcat(strval, ",");

				snprintf(strval, PATH_MAX, "%siso-level=%d", strdup(strval), numval + 1);
			}

			checkbox_get_option(pDlg, "chkISOAllowVernum", "allow-vernum", true, strval);
			checkbox_get_option(pDlg, "chkISOJoliet", "joliet", true, strval);
			checkbox_get_option(pDlg, "chkISOLimitDepth", "limit-depth", true, strval);
			checkbox_get_option(pDlg, "chkISOLimitDirs", "limit-dirs", true, strval);
			checkbox_get_option(pDlg, "chkISOPad", "pad", true, strval);
			checkbox_get_option(pDlg, "chkISORockridge", "rockridge", true, strval);

			textfield_get_option(pDlg, "edISOBoot", "boot", strval);
			textfield_get_option(pDlg, "edISOBootCatalog", "boot-catalog", strval);
			textfield_get_option(pDlg, "cbISOBootType", "boot-type", strval);
			checkbox_get_option(pDlg, "chkISOBootInfoTable", "boot-info-table", false, strval);


			numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cbISOBootType", DM_LISTGETITEMINDEX, 0, 0);

			if (numval > 1)
			{
				gStartupInfo->SendDlgMsg(pDlg, "edISOBootLoadSeg", DM_ENABLE, 0, 0);
				gStartupInfo->SendDlgMsg(pDlg, "edISOBootLoadSize", DM_ENABLE, 0, 0);

			}
			else
			{
				gStartupInfo->SendDlgMsg(pDlg, "edISOBootLoadSeg", DM_ENABLE, 1, 0);
				gStartupInfo->SendDlgMsg(pDlg, "edISOBootLoadSize", DM_ENABLE, 1, 0);
				textfield_get_option(pDlg, "edISOBootLoadSeg", "boot-load-seg", strval);
				textfield_get_option(pDlg, "edISOBootLoadSize", "boot-load-size", strval);
			}

			checkbox_get_option(pDlg, "chkZISOfs", "zisofs", false, strval);
			bval = (bool)gStartupInfo->SendDlgMsg(pDlg, "chkZISOfs", DM_GETCHECK, 0, 0);

			if (bval)
			{
				gStartupInfo->SendDlgMsg(pDlg, "cbZISOCompLvl", DM_ENABLE, 1, 0);

				numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cbZISOCompLvl", DM_LISTGETITEMINDEX, 0, 0);

				if (numval > -1)
				{
					if (strval[strlen(strval) - 1] != ':')
						strcat(strval, ",");

					snprintf(strval, PATH_MAX, "%scompression-level=%d", strdup(strval), numval);
				}
			}
			else
				gStartupInfo->SendDlgMsg(pDlg, "cbZISOCompLvl", DM_ENABLE, 0, 0);

			if (strcmp(strval, "iso9660:") != 0)
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, (intptr_t)strval, 0);
			else
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, 0, 0);

		}
		else if (strstr(DlgItemName, "Filters") != NULL)
		{
			if (strcmp(DlgItemName, "cbFiltersZSTDComprLvl") == 0)
			{
				strncpy(strval, "zstd:", PATH_MAX);

				numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cbFiltersZSTDComprLvl", DM_LISTGETITEMINDEX, 0, 0);

				if (numval > -1)
				{
					gStartupInfo->SendDlgMsg(pDlg, "cbFiltersCompLvl", DM_SETTEXT, 0, 0);
					snprintf(strval, PATH_MAX, "%scompression-level=%d", strdup(strval), numval + 1);
				}
			}
			else if (strstr(DlgItemName, "FiltersLZ4") != NULL)
			{
				strncpy(strval, "lz4:", PATH_MAX);
				checkbox_get_option(pDlg, "chkFiltersLZ4StreamChksum", "stream-checksum", true, strval);
				checkbox_get_option(pDlg, "chkFiltersLZ4BlockChksum", "block-checksum", true, strval);
				checkbox_get_option(pDlg, "chkFiltersLZ4BlockDependence", "block-dependence", false, strval);
				textfield_get_option(pDlg, "cbFiltersLZ4BlockSize", "block-size", strval);
			}
			else if (strcmp(DlgItemName, "chkFiltersGZTimestamp") == 0)
			{
				strncpy(strval, "gzip:", PATH_MAX);
				checkbox_get_option(pDlg, "chkFiltersGZTimestamp", "timestamp", true, strval);
			}
			else if (strcmp(DlgItemName, "edFiltersLZMAThreads") == 0)
			{
				strncpy(strval, "lzma:", PATH_MAX);
				textfield_get_option(pDlg, "edFiltersLZMAThreads", "threads", strval);
			}
			else if (strcmp(DlgItemName, "cbFiltersLrzipCompr") == 0)
			{
				strncpy(strval, "lrzip:", PATH_MAX);
				textfield_get_option(pDlg, "cbFiltersLrzipCompr", "compression", strval);
			}

			numval = (int)gStartupInfo->SendDlgMsg(pDlg, "cbFiltersCompLvl", DM_LISTGETITEMINDEX, 0, 0);

			if (numval > -1)
			{
				if (strval[strlen(strval) - 1] == ':')
					snprintf(strval, PATH_MAX, "compression-level=%d", numval);
				else
				{
					if (strval[strlen(strval) - 1] != '\0')
						strcat(strval, ",");

					snprintf(strval, PATH_MAX, "%scompression-level=%d", strdup(strval), numval);
				}
			}

			if (strval[strlen(strval) - 1] != ':')
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, (intptr_t)strval, 0);
			else
				gStartupInfo->SendDlgMsg(pDlg, "edOptions", DM_SETTEXT, 0, 0);
		}

		break;
	}

	free(strval);
	return 0;
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	tArcData * handle;
	handle = malloc(sizeof(tArcData));

	if (handle == NULL)
	{
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	memset(handle, 0, sizeof(tArcData));
	handle->archive = archive_read_new();

	if (archive_read_set_passphrase_callback(handle->archive, NULL, archive_password_cb) < ARCHIVE_OK)
		errmsg(archive_error_string(handle->archive), MB_OK | MB_ICONERROR);

	archive_read_support_filter_all(handle->archive);
	archive_read_support_filter_program_signature(handle->archive, "lizard -d", lizard_magic, sizeof(lizard_magic));
	archive_read_support_format_raw(handle->archive);
	archive_read_support_format_all(handle->archive);
	strncpy(handle->arcname, ArchiveData->ArcName, PATH_MAX);

	if (gMtreeCheckFS && strcasestr(ArchiveData->ArcName, ".mtree") != NULL)
		archive_read_set_options(handle->archive, "checkfs");
	else if ((gReadDisableJoliet || gReadDisableRockridge) && strcasestr(ArchiveData->ArcName, ".iso") != NULL)
	{
		if (gReadDisableJoliet)
			archive_read_set_options(handle->archive, "!joliet");

		if (gReadDisableRockridge)
			archive_read_set_options(handle->archive, "!rockridge");
	}
	else
	{
		if (gReadIgnoreCRC)
			archive_read_set_options(handle->archive, "ignorecrc32");

		if (gReadMacExt)
			archive_read_set_options(handle->archive, "mac-ext");

		if (gReadCompat2x)
			archive_read_set_options(handle->archive, "compat-2x");

		if (gReadCharset[0] != '\0')
			archive_read_set_format_option(handle->archive, NULL, "hdrcharset", gReadCharset);
	}

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
	char *filename = NULL;

	while ((ret = archive_read_next_header(handle->archive, &handle->entry)) == ARCHIVE_RETRY ||
	                (ret == ARCHIVE_OK && strcmp(".", archive_entry_pathname(handle->entry)) == 0))
	{
		if (ret == ARCHIVE_RETRY && errmsg(archive_error_string(handle->archive),
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
			filename = basename(handle->arcname);

			if (filename)
			{
				char *dot = strrchr(filename, '.');

				if (dot != NULL)
					*dot = '\0';

				strncpy(HeaderDataEx->FileName, filename, sizeof(HeaderDataEx->FileName) - 1);
			}
			else
				strncpy(HeaderDataEx->FileName, "<!!!ERROR!!!>", sizeof(HeaderDataEx->FileName) - 1);
		}
		else
		{
			filename = (char*)archive_entry_pathname(handle->entry);

			if (!filename)
				strncpy(HeaderDataEx->FileName, "<!!!ERROR!!!>", sizeof(HeaderDataEx->FileName) - 1);
			else
			{
				if (filename[0] == '/')
					strncpy(HeaderDataEx->FileName, filename + 1, sizeof(HeaderDataEx->FileName) - 1);
				else if (filename[0] == '.' && filename[1] == '/')
					strncpy(HeaderDataEx->FileName, filename + 2, sizeof(HeaderDataEx->FileName) - 1);
				else
					strncpy(HeaderDataEx->FileName, filename, sizeof(HeaderDataEx->FileName) - 1);
			}

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

	if (Operation == PK_EXTRACT && !DestPath)
	{
		int fd = open(DestName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if (fd == -1)
			return E_ECREATE;
		else
			close(fd);

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
	return PK_CAPS_NEW | PK_CAPS_SEARCHTEXT | PK_CAPS_BY_CONTENT | PK_CAPS_OPTIONS;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	if (access(gLFMPath, F_OK) != 0)
	{
		char *msg;
		asprintf(&msg, "%s\nCurrent Options ($ man archive_write_set_options):", archive_version_details());
		gStartupInfo->InputBox("Double Commander", msg, false, gOptions, PATH_MAX - 1);
		free(msg);
	}
	else
		gStartupInfo->DialogBoxLFMFile(gLFMPath, DlgProc);
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	struct archive_entry *entry;
	struct stat st;
	char buff[BUFF_SIZE];
	int fd, ofd, ret, id;
	ssize_t len;
	char fname[PATH_MAX];
	char infile[PATH_MAX];
	char pkfile[PATH_MAX];
	char link[PATH_MAX + 1];
	int result = E_SUCCESS;
	char *msg, *rmlist, *skiplist, *tmpfn = NULL;
	bool skip_file;
	size_t rsize;
	const void *rbuff;
	la_int64_t roffset;
	struct passwd *pw;
	struct group  *gr;

	const char *ext = strrchr(PackedFile, '.');

	if (!ext)
		return E_NOT_SUPPORTED;

	if (Flags & PK_PACK_MOVE_FILES)
		rmlist = AddList;

	struct archive *a = archive_write_new();

	if ((ret = archive_set_format_filter(a, ext)) == ARCHIVE_WARN)
	{
		printf("libarchive: %s\n", archive_error_string(a));
	}
	else if (ret < ARCHIVE_OK)
	{
		errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
		archive_write_free(a);
		return 0;
	}

	if (access(PackedFile, F_OK) != -1)
	{
		if (errmsg("Options for compression, encryption etc will be LOST. Are you sure you want this?", MB_YESNO | MB_ICONWARNING) != ID_YES)
			result = E_EABORTED;
		else
		{
			strncpy(infile, PackedFile, PATH_MAX);
			tmpfn = tempnam(dirname(infile), "arc_");

			if (access(tmpfn, F_OK) != -1)
				result = E_EWRITE;
			else
			{
				if (archive_format(a) == ARCHIVE_FORMAT_RAW)
				{
					if (strcasestr(PackedFile, ".tar.") == NULL || archive_write_set_format(a, gTarFormat)  < ARCHIVE_OK)
						result = E_NOT_SUPPORTED;
				}

				if (gOptions[0] != '\0')
				{
					asprintf(&msg, "Use these options '%s'?", gOptions);

					if (errmsg(msg, MB_YESNO | MB_ICONQUESTION) == ID_YES)
						if (archive_write_set_options(a, gOptions) < ARCHIVE_OK)
							errmsg(archive_error_string(a), MB_OK | MB_ICONWARNING);

					free(msg);
				}
			}
		}

		if (result == E_SUCCESS)
		{
			struct archive *org = archive_read_new();

			archive_read_support_filter_all(org);
			archive_read_support_filter_program_signature(org, "lizard -d", lizard_magic, sizeof(lizard_magic));
			archive_read_support_format_raw(org);
			archive_read_support_format_all(org);
			archive_read_set_passphrase_callback(org, NULL, archive_password_cb);


			if (archive_read_open_filename(org, PackedFile, 10240) < ARCHIVE_OK)
			{
				errmsg(archive_error_string(org), MB_OK | MB_ICONERROR);
				result = E_EWRITE;
			}

			if (result == E_SUCCESS && (ofd = open(tmpfn, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
				result = E_EWRITE;

			if (result == E_SUCCESS && archive_write_open_fd(a, ofd) < ARCHIVE_OK)
			{
				errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
				result = E_EWRITE;
			}

			if (result == E_SUCCESS)
			{
				while (archive_read_next_header(org, &entry) == ARCHIVE_OK)
				{
					skiplist = AddList;
					strncpy(infile, archive_entry_pathname(entry), PATH_MAX);
					skip_file = false;

					while (*skiplist)
					{
						strncpy(fname, skiplist, PATH_MAX);

						if (!(Flags & PK_PACK_SAVE_PATHS))
							strncpy(fname, strdup(basename(fname)), PATH_MAX);

						if (!SubPath)
							strcpy(pkfile, fname);
						else
							snprintf(pkfile, PATH_MAX, "%s/%s", SubPath, fname);

						if (strncmp(pkfile, infile, PATH_MAX) == 0)
						{
							skip_file = true;
							break;
						}

						while (*skiplist++);
					}

					if (!skip_file)
					{
						archive_write_header(a, entry);

						while (archive_read_data_block(org, &rbuff, &rsize, &roffset) != ARCHIVE_EOF)
						{
							if (archive_write_data(a, rbuff, rsize) < ARCHIVE_OK)
							{
								errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
								result = E_EWRITE;
								break;
							}

							if (gProcessDataProc(tmpfn, 0) == 0)
							{
								result = E_EABORTED;
								break;
							}
						}
					}

				}

				archive_read_close(org);
				archive_read_free(org);
			}
		}

		if (result != E_SUCCESS)
		{
			if (tmpfn)
				free(tmpfn);

			return result;
		}
	}
	else
	{

		if ((gOptions[0] != '\0') && archive_write_set_options(a, gOptions) < ARCHIVE_OK)
		{
			errmsg(archive_error_string(a), MB_OK | MB_ICONWARNING);
		}

		if (archive_filter_code(a, 0) == ARCHIVE_FILTER_UU)
		{
			asprintf(&msg, "name=%s", AddList);
			archive_write_set_options(a, msg);

			strcpy(infile, SrcPath);
			char* pos = strrchr(infile, '/');
			strncpy(fname, AddList, PATH_MAX);

			if (pos != NULL)
				strcpy(pos + 1, fname);
			else
				strcpy(infile, fname);

			if (stat(infile, &st) == 0)
			{
				asprintf(&msg, "mode=%o", st.st_mode);
				archive_write_set_options(a, msg);
			}

			free(msg);
		}

		archive_write_set_passphrase_callback(a, NULL, archive_password_cb);

		if (Flags & PK_PACK_ENCRYPT)
		{
			if (archive_write_set_options(a, gEncryption) < ARCHIVE_OK)
				errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
		}

		ofd = open(PackedFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if (ofd == -1)
			result = E_ECREATE;
		else if (archive_write_open_fd(a, ofd) < ARCHIVE_OK)
		{
			errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
			result = E_ECREATE;
		}
	}

	if (result == E_SUCCESS)
	{

		struct archive *disk = archive_read_disk_new();

		archive_read_disk_set_standard_lookup(disk);

		archive_read_disk_set_symlink_physical(disk);

		while (*AddList)
		{
			strcpy(infile, SrcPath);
			char* pos = strrchr(infile, '/');
			strncpy(fname, AddList, PATH_MAX);

			if (pos != NULL)
				strcpy(pos + 1, fname);
			else
				strcpy(infile, fname);

			if (!(Flags & PK_PACK_SAVE_PATHS))
				strncpy(fname, strdup(basename(fname)), PATH_MAX);

			if (!SubPath)
				strcpy(pkfile, fname);
			else
				snprintf(pkfile, PATH_MAX, "%s/%s", SubPath, fname);

			if (gProcessDataProc(infile, 0) == 0)
				result = E_EABORTED;

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

			if ((ret == 0) && !(S_ISDIR(st.st_mode) && !(Flags & PK_PACK_SAVE_PATHS)))
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

					if ((gOptions[0] == '\0') || (strstr(gOptions, "!flags") == NULL))
					{
						if (S_ISFIFO(st.st_mode))
						{
							asprintf(&msg, "%s: ignoring flags for named pipe.", infile);

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

					if (st.st_size < INT_MAX && gProcessDataProc(infile, st.st_size) == 0)
						result = E_EABORTED;
				}
				else if (S_ISFIFO(st.st_mode))
				{
					asprintf(&msg, "%s: ignoring named pipe.", infile);

					if (errmsg(msg, MB_OKCANCEL | MB_ICONWARNING) == ID_CANCEL)
						result = E_EABORTED;

					free(msg);
				}
				else
				{

					while ((fd = open(infile, O_RDONLY)) == -1 && !S_ISLNK(st.st_mode))
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
					else if (S_ISLNK(st.st_mode) && result != E_EABORTED)
					{
						archive_entry_copy_stat(entry, &st);
						archive_entry_set_pathname(entry, pkfile);
						pw = getpwuid(st.st_uid);
						gr = getgrgid(st.st_gid);

						if (gr)
							archive_entry_set_gname(entry, gr->gr_name);

						if (pw)
							archive_entry_set_uname(entry, pw->pw_name);

						if ((len = readlink(pkfile, link, sizeof(link) - 1)) != -1)
						{
							link[len] = '\0';
							archive_entry_set_symlink(entry, link);
						}
						else
							archive_entry_set_symlink(entry, "");

						archive_write_header(a, entry);

						if (st.st_size < INT_MAX)
							gProcessDataProc(infile, st.st_size);
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
	}

	archive_write_close(a);
	archive_write_free(a);

	if (ofd > -1)
		close(ofd);

	if ((Flags & PK_PACK_MOVE_FILES && result == E_SUCCESS) &&
	                (errmsg("Now WILL TRY to REMOVE ALL (including SKIPPED!) source files. Are you sure you want this?", MB_YESNO | MB_ICONWARNING) == ID_YES))
	{
		while (*rmlist)
		{
			strcpy(infile, SrcPath);
			char* pos = strrchr(infile, '/');

			if (pos != NULL)
				strcpy(pos + 1, rmlist);

			remove_target(infile);

			while (*rmlist++);
		}
	}

	if (tmpfn)
	{
		if (result == E_SUCCESS)
			rename(tmpfn, PackedFile);

		free(tmpfn);
	}

	return result;
}

int DCPCALL DeleteFiles(char *PackedFile, char *DeleteList)
{
	size_t size;
	la_int64_t offset;
	const void *buff;
	bool skip_file;
	char infile[PATH_MAX];
	char rmfile[PATH_MAX];
	char *msg, *rmlist, *tmpfn = NULL;
	struct archive_entry *entry;
	int ofd, ret, result = E_SUCCESS;

	const char *ext = strrchr(PackedFile, '.');

	struct archive *a = archive_write_new();

	if ((ret = archive_set_format_filter(a, ext)) == ARCHIVE_WARN)
	{
		printf("libarchive: %s\n", archive_error_string(a));
	}
	else if (ret < ARCHIVE_OK)
	{
		errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
		archive_write_free(a);
		return 0;
	}

	if (errmsg("Options for compression, encryption etc will be LOST. Are you sure you want this?", MB_YESNO | MB_ICONWARNING) != ID_YES)
		result = E_EABORTED;
	else
	{
		strncpy(infile, PackedFile, PATH_MAX);
		tmpfn = tempnam(dirname(infile), "arc_");

		if (access(tmpfn, F_OK) != -1)
			result = E_EWRITE;
		else
		{
			if (archive_format(a) == ARCHIVE_FORMAT_RAW)
			{
				if (strcasestr(PackedFile, ".tar.") == NULL || archive_write_set_format(a, gTarFormat)  < ARCHIVE_OK)
					result = E_NOT_SUPPORTED;
			}

			if (gOptions[0] != '\0')
			{
				asprintf(&msg, "Use these options '%s'?", gOptions);

				if (errmsg(msg, MB_YESNO | MB_ICONQUESTION) == ID_YES)
					if (archive_write_set_options(a, gOptions) < ARCHIVE_OK)
						errmsg(archive_error_string(a), MB_OK | MB_ICONWARNING);

				free(msg);
			}
		}
	}

	if (result == E_SUCCESS)
	{
		struct archive *org = archive_read_new();

		archive_read_support_filter_all(org);
		archive_read_support_filter_program_signature(org, "lizard -d", lizard_magic, sizeof(lizard_magic));
		archive_read_support_format_raw(org);
		archive_read_support_format_all(org);
		archive_read_set_passphrase_callback(org, NULL, archive_password_cb);

		if (archive_read_open_filename(org, PackedFile, 10240) < ARCHIVE_OK)
		{
			errmsg(archive_error_string(org), MB_OK | MB_ICONERROR);
			result = E_EWRITE;
		}

		if (result == E_SUCCESS && (ofd = open(tmpfn, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
			result = E_EWRITE;

		if (result == E_SUCCESS && archive_write_open_fd(a, ofd) < ARCHIVE_OK)
		{
			errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
			result = E_EWRITE;
		}

		if (result == E_SUCCESS)
		{
			while (archive_read_next_header(org, &entry) == ARCHIVE_OK)
			{
				skip_file = false;
				rmlist = DeleteList;
				strncpy(infile, archive_entry_pathname(entry), PATH_MAX);

				while (*rmlist)
				{
					strncpy(rmfile, rmlist, PATH_MAX);

					if (strncmp(rmfile + strlen(rmfile) - 4, "/*.*", 4) == 0)
						rmfile[strlen(rmfile) - 2] = 0;

					if (fnmatch(rmfile, infile, FNM_CASEFOLD) == 0)
					{
						skip_file = true;
						break;
					}

					while (*rmlist++);
				}

				if (!skip_file)
				{
					archive_write_header(a, entry);

					while (archive_read_data_block(org, (const void**)&buff, &size, &offset) != ARCHIVE_EOF)
					{
						if (archive_write_data(a, buff, size) < ARCHIVE_OK)
						{
							errmsg(archive_error_string(a), MB_OK | MB_ICONERROR);
							result = E_EWRITE;
							break;
						}

						if (gProcessDataProc(tmpfn, 0) == 0)
						{
							result = E_EABORTED;
							break;
						}
					}
				}
			}

			archive_read_close(org);
			archive_read_free(org);
			archive_write_finish_entry(a);
			archive_write_close(a);
			archive_write_free(a);

			if (ofd > -1)
				close(ofd);

		}
	}

	if (result == E_SUCCESS)
		rename(tmpfn, PackedFile);

	if (tmpfn)
		free(tmpfn);

	return result;
}
