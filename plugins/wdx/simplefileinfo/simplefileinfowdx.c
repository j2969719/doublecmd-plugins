#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <magic.h>
#include "wdxplugin.h"

#define _detectstring "EXT=\"*\""

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))
#define iflags (FS_APPEND_FL|FS_COMPR_FL|FS_DIRSYNC_FL|FS_IMMUTABLE_FL|FS_JOURNAL_DATA_FL|FS_NOATIME_FL|FS_NOCOW_FL|FS_NODUMP_FL|FS_NOTAIL_FL|FS_PROJINHERIT_FL|FS_SECRM_FL|FS_SYNC_FL|FS_TOPDIR_FL|FS_UNRM_FL)

#define info_units "default|fast|folow symlinks|uncompress|uncompress and folow symlinks|all matches"
#define flags_units  "GETFLAGS|APPEND|COMPR|DIRSYNC|IMMUTABLE|JOURNAL_DATA|NOATIME|NOCOW|NODUMP|NOTAIL|PROJINHERIT|SECRM|SYNC|TOPDIR|UNRM"

FIELD fields[] =
{
	{"Info",			ft_string,				info_units},
	{"MIME type",			ft_string,		  "default|folow symlinks"},
	{"MIME encoding",		ft_string,		  "default|folow symlinks"},
	{"Inode flags",			ft_string,					""},
	{"Access for current user",	ft_string,				"---|----"},
	{"Access rights in octal",	ft_numeric_32,				"xxxx|xxx"},
	{"User name",			ft_string,					""},
	{"User ID",			ft_numeric_32,					""},
	{"Group name",			ft_string,					""},
	{"Group ID",			ft_numeric_32,					""},
	{"Inode number",		ft_numeric_32,					""},
	{"Size",			ft_numeric_64,					""},
	{"Block size",			ft_numeric_32,					""},
	{"Number of blocks",		ft_numeric_64,					""},
	{"Number of hard links",	ft_numeric_32,					""},
	{"User access",			ft_boolean,	     "open dir|read|write|execute"},
	{"Symlink error",		ft_boolean,		 "no access|dangling|loop"},
	{"Inode flags (bool)",		ft_boolean,		 	       flags_units},
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

	strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	strlcpy(Units, fields[FieldIndex].unit, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	strlcpy(DetectString, _detectstring, maxlen - 1);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	struct stat buf;
	const char *magic_full;
	magic_t magic_cookie;
	mode_t mode_bits;
	char access_str[4] = "----";
	char flags_str[14] = "--------------";
	int access_how, fd, stflags, i;

	if (lstat(FileName, &buf) != 0)
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
	{
		if (S_ISFIFO(buf.st_mode) || (fd = open(FileName, O_RDONLY)) == -1)
			return ft_fileerror;
		else
		{
			i=0;

			if (ioctl(fd, FS_IOC_GETFLAGS, &stflags) == 0 && stflags != 0)
			{
				if (stflags & FS_APPEND_FL)
					flags_str[i] = 'a';

				i++;

				if (stflags & FS_COMPR_FL)
					flags_str[i] = 'c';

				i++;

				if (stflags & FS_DIRSYNC_FL)
					flags_str[i] = 'D';

				i++;

				if (stflags & FS_IMMUTABLE_FL)
					flags_str[i] = 'i';

				i++;

				if (stflags &  FS_JOURNAL_DATA_FL)
					flags_str[i] = 'j';

				i++;

				if (stflags & FS_NOATIME_FL)
					flags_str[i] = 'A';

				i++;

				if (stflags & FS_NOCOW_FL)
					flags_str[i] = 'C';

				i++;

				if (stflags & FS_NODUMP_FL)
					flags_str[i] = 'd';

				i++;

				if (stflags & FS_NOTAIL_FL)
					flags_str[i] = 't';

				i++;

				if (stflags & FS_PROJINHERIT_FL)
					flags_str[i] = 'P';

				i++;

				if (stflags & FS_SECRM_FL)
					flags_str[i] = 's';

				i++;

				if (stflags & FS_SYNC_FL)
					flags_str[i] = 'S';

				i++;

				if (stflags & FS_TOPDIR_FL)
					flags_str[i] = 'T';

				i++;

				if (stflags & FS_UNRM_FL)
					flags_str[i] = 'u';

			}

			close(fd);
		}

		strlcpy((char*)FieldValue, flags_str, maxlen - 1);
		break;
	}

	case 4:
	{
		i = 0;

		if (UnitIndex == 1)
		{
			if (S_ISDIR(buf.st_mode))
				access_str[i] = 'd';
			else if (S_ISCHR(buf.st_mode))
				access_str[i] = 'c';
			else if (S_ISBLK(buf.st_mode))
				access_str[i] = 'b';
			else if (S_ISFIFO(buf.st_mode))
				access_str[i] = 'f';
			else if (S_ISLNK(buf.st_mode))
				access_str[i] = 'l';
			else if (S_ISSOCK(buf.st_mode))
				access_str[i] = 's';

			i++;
		}

		if (access(FileName, R_OK) == 0)
			access_str[i] = 'r';

		i++;

		if (access(FileName, W_OK) == 0)
			access_str[i] = 'w';

		i++;

		if (access(FileName, X_OK) == 0)
			access_str[i] = 'x';

		i++;
		access_str[i] = '\0';

		strlcpy((char*)FieldValue, access_str, maxlen - 1);

		break;
	}

	case 5:
		if (UnitIndex == 0)
			mode_bits = S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX;
		else
			mode_bits = S_IRWXU | S_IRWXG | S_IRWXO;

		*(int*)FieldValue = convertDecimalToOctal(buf.st_mode & mode_bits);

		break;

	case 6:
		if (pw)
			strncpy((char*)FieldValue, pw->pw_name, maxlen - 1);
		else
			return ft_fieldempty;

		break;

	case 7:
		*(int*)FieldValue = buf.st_uid;
		break;

	case 8:
		if (gr)
			strncpy((char*)FieldValue, gr->gr_name, maxlen - 1);
		else
			return ft_fieldempty;

		break;

	case 9:
		*(int*)FieldValue = buf.st_gid;
		break;

	case 10:
		*(int*)FieldValue = buf.st_ino;
		break;

	case 11:
		*(int*)FieldValue = buf.st_size;
		break;

	case 12:
		*(int*)FieldValue = buf.st_blksize;
		break;

	case 13:
		*(int*)FieldValue = buf.st_blocks;
		break;

	case 14:
		*(int*)FieldValue = buf.st_nlink;
		break;

	case 15:
	{
		switch (UnitIndex)
		{
		case 1:
			access_how = R_OK;
			break;

		case 2:
			access_how = W_OK;
			break;

		case 3:
			access_how = X_OK;
			break;

		default:
			if (S_ISDIR(buf.st_mode))
				access_how = R_OK | X_OK;
			else
				return ft_fileerror;
		}

		if (access(FileName, access_how) == 0)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;
	}

	case 16:
	{
		if (S_ISLNK(buf.st_mode))
		{
			*(int*)FieldValue = 0;

			if (access(FileName, F_OK) == -1)
			{
				int errsv = errno;

				switch (UnitIndex)
				{
				case 1:
				{
					if (errsv == ENOENT)
						*(int*)FieldValue = 1;

					break;
				}

				case 2:
				{
					if (errsv == ELOOP)
						*(int*)FieldValue = 1;

					break;
				}

				default:
					*(int*)FieldValue = 1;
				}
			}
			else
				return ft_fieldempty;
		}
		else
			return ft_fileerror;

		break;
	}

	case 17:
	{
		*(int*)FieldValue = 0;

		if (S_ISFIFO(buf.st_mode) || (fd = open(FileName, O_RDONLY)) == -1)
		{
			if (UnitIndex > 0)
				return ft_fileerror;
		}
		else
		{
			if (ioctl(fd, FS_IOC_GETFLAGS, &stflags) == 0 && stflags != 0)
			{
				switch (UnitIndex)
				{
				case 1:
				{
					if (stflags & FS_APPEND_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 2:
				{
					if (stflags & FS_COMPR_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 3:
				{
					if (stflags & FS_DIRSYNC_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 4:
				{
					if (stflags & FS_IMMUTABLE_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 5:
				{
					if (stflags &  FS_JOURNAL_DATA_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 6:
				{
					if (stflags & FS_NOATIME_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 7:
				{
					if (stflags & FS_NOCOW_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 8:
				{
					if (stflags & FS_NODUMP_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 9:
				{
					if (stflags & FS_NOTAIL_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 10:
				{
					if (stflags & FS_PROJINHERIT_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 11:
				{
					if (stflags & FS_SECRM_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 12:
				{
					if (stflags & FS_SYNC_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 13:
				{
					if (stflags & FS_TOPDIR_FL)
						*(int*)FieldValue = 1;

					break;
				}

				case 14:
				{
					if (stflags & FS_UNRM_FL)
						*(int*)FieldValue = 1;

					break;
				}

				default:
					if (stflags & iflags)
					*(int*)FieldValue = 1;
				}
			}

			close(fd);
		}

		break;
	}

	default:
		return ft_nosuchfield;
	}

	if ((FieldIndex >= 0) && (FieldIndex < 3))
	{
		if (magic_cookie == NULL)
		{
			printf("unable to initialize magic library\n");
			return ft_fileerror;
		}

		if (magic_load(magic_cookie, NULL) != 0)
		{
			printf("cannot load magic database - %s\n", magic_error(magic_cookie));
			magic_close(magic_cookie);
			return ft_fileerror;
		}

		magic_full = magic_file(magic_cookie, FileName);

		if (magic_full)
			strlcpy((char*)FieldValue, magic_full, maxlen - 1);
		else
			return ft_fieldempty;

		magic_close(magic_cookie);
	}

	return fields[FieldIndex].type;
}
