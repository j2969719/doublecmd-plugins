#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <math.h>
#include <magic.h>
#include <linux/limits.h>
#include "wdxplugin.h"

#include <dlfcn.h>

#include <libintl.h>
#include <locale.h>

#define N_(String) String
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#define _detectstring "EXT=\"*\""

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{N_("Info"),			ft_string,	N_("default|fast|folow symlinks|uncompress|uncompress and folow symlinks|all matches")},
	{N_("MIME type"),		ft_string,								  N_("default|folow symlinks")},
	{N_("MIME encoding"),		ft_string,								  N_("default|folow symlinks")},
	{N_("Object type"),		ft_multiplechoice,	  N_("file|directory|character device|block device|named pipe|symlink|socket")},
	{N_("Access rights in octal"),	ft_numeric_32,										    "xxxx|xxx"},
	{N_("User name"),		ft_string,											    ""},
	{N_("User ID"),			ft_numeric_32,											    ""},
	{N_("Group name"),		ft_string,											    ""},
	{N_("Group ID"),		ft_numeric_32,											    ""},
	{N_("Inode number"),		ft_numeric_32,											    ""},
	{N_("Size"),			ft_numeric_64,											    ""},
	{N_("Block size"),		ft_numeric_32,											    ""},
	{N_("Number of blocks"),	ft_numeric_64,											    ""},
	{N_("Number of hard links"),	ft_numeric_32,											    ""},
	{N_("Mountpoint"),		ft_boolean,											    ""},
};

char* objtypevalue[7] =
{
	N_("file"),
	N_("directory"),
	N_("character device"),
	N_("block device"),
	N_("named pipe"),
	N_("symlink"),
	N_("socket")
};

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
int convertDecimalToOctal(int decimalNumber)
{
	int octalNumber = 0, i = 1;

	while (decimalNumber != 0)
	{
		octalNumber += (decimalNumber % 8) * i;
		decimalNumber /= 8;
		i *= 10;
	}

	return octalNumber;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	strncpy(FieldName, gettext(fields[FieldIndex].name), maxlen - 1);

	if (fields[FieldIndex].unit != "")
		strncpy(Units, gettext(fields[FieldIndex].unit), maxlen - 1);
	else
		strncpy(Units, "", maxlen - 1);

	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	strlcpy(DetectString, _detectstring, maxlen-1);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	char tname[PATH_MAX], pname[PATH_MAX + 4];
	struct stat buf, bfparent;
	const char *magic_full;
	magic_t magic_cookie;
	strlcpy(tname, FileName + strlen(FileName) - 3, 4);

	if (strcmp(tname, "/..") == 0)
		strlcpy(tname, FileName, strlen(FileName) - 2);
	else
		strlcpy(tname, FileName, strlen(FileName) + 1);

	if (lstat(tname, &buf) != 0)
		return ft_fileerror;

	struct passwd *pw = getpwuid(buf.st_uid);
	struct group  *gr = getgrgid(buf.st_gid);

	switch (FieldIndex)
	{
	case 0:
		switch (UnitIndex)
		{
		case 1:
			magic_cookie = magic_open(MAGIC_NO_CHECK_SOFT);
			break;

		case 2:
			magic_cookie = magic_open(MAGIC_SYMLINK);
			break;

		case 3:
			magic_cookie = magic_open(MAGIC_COMPRESS);;
			break;

		case 4:
			magic_cookie = magic_open(MAGIC_SYMLINK | MAGIC_COMPRESS);
			break;

		case 5:
			magic_cookie = magic_open(MAGIC_CONTINUE | MAGIC_SYMLINK | MAGIC_COMPRESS);
			break;

		default:
			magic_cookie = magic_open(MAGIC_NONE);
		}

		break;

	case 1:
		if (UnitIndex == 0)
			magic_cookie = magic_open(MAGIC_MIME_TYPE);
		else
			magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);

		break;

	case 2:
		if (UnitIndex == 0)
			magic_cookie = magic_open(MAGIC_MIME_ENCODING);
		else
			magic_cookie = magic_open(MAGIC_MIME_ENCODING | MAGIC_SYMLINK);

		break;

	case 3:
		if (S_ISDIR(buf.st_mode))
			strlcpy((char*)FieldValue, gettext(objtypevalue[1]), maxlen - 1);
		else if (S_ISCHR(buf.st_mode))
			strlcpy((char*)FieldValue, gettext(objtypevalue[2]), maxlen - 1);
		else if (S_ISBLK(buf.st_mode))
			strlcpy((char*)FieldValue, gettext(objtypevalue[3]), maxlen - 1);
		else if (S_ISFIFO(buf.st_mode))
			strlcpy((char*)FieldValue, gettext(objtypevalue[4]), maxlen - 1);
		else if (S_ISLNK(buf.st_mode))
			strlcpy((char*)FieldValue, gettext(objtypevalue[5]), maxlen - 1);
		else if (S_ISSOCK(buf.st_mode))
			strlcpy((char*)FieldValue, gettext(objtypevalue[6]), maxlen - 1);
		else
			strlcpy((char*)FieldValue, gettext(objtypevalue[0]), maxlen - 1);

		break;

	case 4:
		if (UnitIndex == 0)
			*(int*)FieldValue = convertDecimalToOctal(buf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX));
		else
			*(int*)FieldValue = convertDecimalToOctal(buf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));

		break;

	case 5:
		strncpy((char*)FieldValue, pw->pw_name, maxlen - 1);
		break;

	case 6:
		*(int*)FieldValue = buf.st_uid;
		break;

	case 7:
		strncpy((char*)FieldValue, gr->gr_name, maxlen - 1);
		break;

	case 8:
		*(int*)FieldValue = buf.st_gid;
		break;

	case 9:
		*(int*)FieldValue = buf.st_ino;
		break;

	case 10:
		*(int*)FieldValue = buf.st_size;
		break;

	case 11:
		*(int*)FieldValue = buf.st_blksize;
		break;

	case 12:
		*(int*)FieldValue = buf.st_blocks;
		break;

	case 13:
		*(int*)FieldValue = buf.st_nlink;
		break;

	case 14:
		if (strcmp(tname, "/") == 0)
			return ft_fileerror;

		if (S_ISDIR(buf.st_mode))
		{
			sprintf(pname, "%s/..", tname);

			if (lstat(pname, &bfparent) != 0)
				return ft_nosuchfield;

			if ((buf.st_dev == bfparent.st_dev) && (buf.st_ino != bfparent.st_ino))
			{
				*(int*)FieldValue = 0;
			}
			else
			{
				*(int*)FieldValue = 1;
			}
		}
		else
		{
			return ft_fieldempty;
		}

		break;

	default:
		return ft_nosuchfield;
	}

	if ((FieldIndex >= 0) && (FieldIndex < 3))
	{
		if (magic_cookie == NULL)
		{
			printf(_("unable to initialize magic library\n"));
			return ft_fileerror;
		}

		if (magic_load(magic_cookie, NULL) != 0)
		{
			printf(_("cannot load magic database - %s\n"), magic_error(magic_cookie));
			magic_close(magic_cookie);
			return ft_fileerror;
		}

		magic_full = magic_file(magic_cookie, tname);

		if (magic_full)
			strncpy((char*)FieldValue, magic_full, maxlen - 1);
		else
			return ft_fieldempty;

		magic_close(magic_cookie);
	}

	return fields[FieldIndex].type;

}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	static char plg_path[PATH_MAX];
	const char* loc_dir = "langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(plg_path, &dlinfo) != 0)
	{
		strncpy(plg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plg_path, '/');

		if (pos)
			strcpy(pos + 1, loc_dir);

		setlocale (LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, plg_path);
		textdomain(GETTEXT_PACKAGE);
	}
}
