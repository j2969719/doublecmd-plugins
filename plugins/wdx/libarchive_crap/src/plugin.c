#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <archive.h>
#include <archive_entry.h>
#include <string.h>
#include "wdxplugin.h"


typedef struct sfield
{
	char *name;
	int type;
	int64_t val;
} tfield;

#define fieldcount (sizeof(fields)/sizeof(tfield))

tfield fields[] =
{
	{"archive",		ft_multiplechoice,	0},
	{"~totalsize",		ft_numeric_64,		0},
	{"items count",		ft_numeric_64,		0},
	{"files",		ft_numeric_64,		0},
	{"folders",		ft_numeric_64,		0},
	{"symlinks",		ft_numeric_64,		0},
	{"other",		ft_numeric_64,		0},
	{"filtres count",	ft_numeric_64,		0},
	{"gzip",		ft_boolean,		0},
	{"bzip2",		ft_boolean,		0},
	{"ms compress",		ft_boolean,		0},
	{"lzma",		ft_boolean,		0},
	{"xz",			ft_boolean,		0},
	{"uu",			ft_boolean,		0},
	{"rpm",			ft_boolean,		0},
	{"lzip",		ft_boolean,		0},
	{"lrzip",		ft_boolean,		0},
	{"lzop",		ft_boolean,		0},
	{"grzip",		ft_boolean,		0},
	{"lz4",			ft_boolean,		0},
	{"zstd",		ft_boolean,		0},
	{"symlinks_undef",	ft_numeric_64,		0},
	{"symlinks_file",	ft_numeric_64,		0},
	{"symlinks_dir",	ft_numeric_64,		0},
	{"warnings",		ft_numeric_64,		0},
};

enum fieldnum
{
	F_ARCTYPE,
	F_TOTALSIZE,
	F_TOTALCOUNT,
	F_FILES,
	F_FOLDERS,
	F_SYMLINKS,
	F_OTHER,
	F_FILTESCOUNT,
	F_BGZIP,
	F_BBZIP2,
	F_BCOMPRESS,
	F_BLZMA,
	F_BXZ,
	F_BUU,
	F_BRPM,
	F_BLZIP,
	F_BLRZIP,
	F_BLZOP,
	F_BGRZIP,
	F_BLZ4,
	F_BZSTD,
	F_SYMLINKS_UNDEF,
	F_SYMLINKS_FILE,
	F_SYMLINKS_DIR,
	F_WARNCOUNT
};

const char *archive_multichoice[] =
{
	"Zip",
	"7Zip",
	"RAR",
	"TAR",
	"CPIO",
	"CAB",
	"LHA",
	"AR",
	"SHAR",
	"XAR",
	"WARC"
};

enum multichoicenum
{
	M_ZIP,
	M_7ZIP,
	M_RAR,
	M_TAR,
	M_CPIO,
	M_CAB,
	M_LHA,
	M_AR,
	M_SHAR,
	M_XAR,
	M_WARC
};

char lastfile[PATH_MAX];
bool lastfile_ok = false;

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

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	memset(Units, 0, maxlen);

	if (fields[FieldIndex].type == ft_multiplechoice)
	{
		const char **p = archive_multichoice;

		int len = 0;

		while (*p != NULL)
		{
			if (len > 0 && len + sizeof(char) < maxlen - 1)
				strcat(Units, "|");

			len += strlen(*p);

			if (len < maxlen - 1)
				strcat(Units, *p);

			*p++;
		}
	}

	return fields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int r;
	struct archive *a;
	struct archive_entry *entry;
	struct stat buf;
	__LA_MODE_T mode;

	if (stat(FileName, &buf) != 0 || !S_ISREG(buf.st_mode))
		return ft_fileerror;

	if (strcmp(FileName, lastfile) != 0)
	{
		lastfile_ok = true;

		for (int i = 0; i < fieldcount; i++)
			fields[i].val = 0;

		strlcpy(lastfile, FileName, PATH_MAX);
		a = archive_read_new();
		archive_read_support_filter_all(a);
		archive_read_support_format_all(a);
		r = archive_read_open_filename(a, FileName, 10240);

		if (r != ARCHIVE_OK)
			lastfile_ok = false;
		else
		{

			while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK || r == ARCHIVE_WARN)
			{
				fields[F_TOTALCOUNT].val++;

				if (r == ARCHIVE_WARN)
					fields[F_WARNCOUNT].val++;

				fields[F_TOTALSIZE].val += archive_entry_size(entry);
				mode = archive_entry_filetype(entry);

				switch (mode & AE_IFMT)
				{
				case AE_IFREG:
					fields[F_FILES].val++;
					break;

				case AE_IFDIR:
					fields[F_FOLDERS].val++;
					break;

				case AE_IFLNK:
					fields[F_SYMLINKS].val++;

					switch (archive_entry_symlink_type(entry))
					{
					case AE_SYMLINK_TYPE_UNDEFINED:
						fields[F_SYMLINKS_UNDEF].val++;
						break;

					case AE_SYMLINK_TYPE_FILE:
						fields[F_SYMLINKS_FILE].val++;
						break;

					case AE_SYMLINK_TYPE_DIRECTORY:
						fields[F_SYMLINKS_DIR].val++;
						break;
					}

					break;

				default:
					fields[F_OTHER].val++;
				}

			}

			fields[F_ARCTYPE].val = archive_format(a);
			fields[F_FILTESCOUNT].val = archive_filter_count(a) - 1;

			for (int i = 0; i <= fields[F_FILTESCOUNT].val; i++)
			{
				int filter_code = archive_filter_code(a, i);

				switch (filter_code)
				{
				case ARCHIVE_FILTER_GZIP:
					fields[F_BGZIP].val++;
					break;

				case ARCHIVE_FILTER_BZIP2:
					fields[F_BBZIP2].val++;
					break;

				case ARCHIVE_FILTER_COMPRESS:
					fields[F_BCOMPRESS].val++;
					break;

				case ARCHIVE_FILTER_LZMA:
					fields[F_BLZMA].val++;
					break;

				case ARCHIVE_FILTER_XZ:
					fields[F_BXZ].val++;
					break;

				case ARCHIVE_FILTER_UU:
					fields[F_BUU].val++;
					break;

				case ARCHIVE_FILTER_RPM:
					fields[F_BRPM].val++;
					break;

				case ARCHIVE_FILTER_LZIP:
					fields[F_BLZIP].val++;
					break;

				case ARCHIVE_FILTER_LRZIP:
					fields[F_BLRZIP].val++;
					break;

				case ARCHIVE_FILTER_LZOP:
					fields[F_BLZOP].val++;
					break;

				case ARCHIVE_FILTER_GRZIP:
					fields[F_BGRZIP].val++;
					break;

				case ARCHIVE_FILTER_LZ4:
					fields[F_BLZ4].val++;
					break;

				case ARCHIVE_FILTER_ZSTD:
					fields[F_BZSTD].val++;
					break;
				}
			}

			if (fields[F_ARCTYPE].val == ARCHIVE_FORMAT_EMPTY || fields[F_ARCTYPE].val == 0)
				lastfile_ok = false;

			if (r != ARCHIVE_EOF)
				lastfile_ok = false;
		}

		archive_read_close(a);
		archive_read_free(a);

	}

	if (lastfile_ok == false)
		return ft_fileerror;

	if (fields[FieldIndex].type == ft_numeric_64)
		*(int64_t*)FieldValue = fields[FieldIndex].val;
	else if (fields[FieldIndex].type == ft_multiplechoice)
	{
		int index = -1;

		switch (fields[F_ARCTYPE].val)
		{
		case ARCHIVE_FORMAT_ZIP:
			index = M_ZIP;
			break;

		case ARCHIVE_FORMAT_CPIO:
			index = M_CPIO;
			break;

		case ARCHIVE_FORMAT_CPIO_POSIX:
			index = M_CPIO;
			break;

		case ARCHIVE_FORMAT_CPIO_BIN_LE:
			index = M_CPIO;
			break;

		case ARCHIVE_FORMAT_CPIO_BIN_BE:
			index = M_CPIO;
			break;

		case ARCHIVE_FORMAT_CPIO_SVR4_NOCRC:
			index = M_CPIO;
			break;

		case ARCHIVE_FORMAT_CPIO_SVR4_CRC:
			index = M_CPIO;
			break;

		case ARCHIVE_FORMAT_CPIO_AFIO_LARGE:
			index = M_CPIO;
			break;

		case ARCHIVE_FORMAT_SHAR:
			index = M_SHAR;
			break;

		case ARCHIVE_FORMAT_SHAR_BASE:
			index = M_SHAR;
			break;

		case ARCHIVE_FORMAT_SHAR_DUMP:
			index = M_SHAR;
			break;

		case ARCHIVE_FORMAT_TAR:
			index = M_TAR;
			break;

		case ARCHIVE_FORMAT_TAR_USTAR:
			index = M_TAR;
			break;

		case ARCHIVE_FORMAT_TAR_PAX_INTERCHANGE:
			index = M_TAR;
			break;

		case ARCHIVE_FORMAT_TAR_PAX_RESTRICTED:
			index = M_TAR;
			break;

		case ARCHIVE_FORMAT_TAR_GNUTAR:
			index = M_TAR;
			break;

		case ARCHIVE_FORMAT_AR:
			index = M_AR;
			break;

		case ARCHIVE_FORMAT_AR_GNU:
			index = M_AR;
			break;

		case ARCHIVE_FORMAT_AR_BSD:
			index = M_AR;
			break;

		case ARCHIVE_FORMAT_XAR:
			index = M_XAR;
			break;

		case ARCHIVE_FORMAT_LHA:
			index = M_LHA;
			break;

		case ARCHIVE_FORMAT_CAB:
			index = M_CAB;
			break;

		case ARCHIVE_FORMAT_RAR:
			index = M_RAR;
			break;

		case ARCHIVE_FORMAT_RAR_V5:
			index = M_RAR;
			break;

		case ARCHIVE_FORMAT_7ZIP:
			index = M_7ZIP;
			break;

		case ARCHIVE_FORMAT_WARC:
			index = M_WARC;
			break;

		default:
			return ft_fieldempty;
		}

		strlcpy((char*)FieldValue, archive_multichoice[index], maxlen - 1);
	}
	else if (fields[FieldIndex].type == ft_boolean)
		*(int*)FieldValue = (int)fields[FieldIndex].val;
	else
		return ft_nosuchfield;

	return fields[FieldIndex].type;
}
