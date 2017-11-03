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

#define _detectstring "EXT=\"*\""

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[]={
	{"Info",					ft_string,																			""},
	{"Mode",					ft_string,	"fast|folow symlinks|uncompress|uncompress and folow symlinks|all matches"},
	{"MIME type",				ft_string,																			""},
	{"MIME encoding",			ft_string,																			""},
	{"Object type",				ft_boolean,	"file|directory|character device|block device|named pipe|symlink|socket"},
	{"Access rights in octal",	ft_numeric_32,																		""},
	{"User name",				ft_string,																			""},
	{"User ID",					ft_numeric_32,																		""},
	{"Group name",				ft_string,																			""},
	{"Group ID",				ft_numeric_32,																		""},
	{"Inode number",			ft_numeric_32,																		""},
	{"Block size",				ft_numeric_32,																		""},
	{"Number of blocks",		ft_numeric_32,																		""},
	{"Number of hard links",	ft_numeric_32,																		""},
	{"Mountpoint",				ft_boolean,																			""},
};

char* strlcpy(char* p,const char* p2,int maxlen)
{
		if ((int)strlen(p2)>=maxlen)
		{
			strncpy(p,p2,maxlen);
			p[maxlen]=0;
		} else
			strcpy(p,p2);
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

int DCPCALL ContentGetSupportedField(int FieldIndex,char* FieldName,char* Units,int maxlen)
{
		if (FieldIndex<0 || FieldIndex>=fieldcount)
			return ft_nomorefields;
		strncpy(FieldName,fields[FieldIndex].name,maxlen-1);
		strncpy(Units,fields[FieldIndex].unit,maxlen-1);
		return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString,int maxlen)
{
		strlcpy(DetectString,_detectstring,maxlen);
		return 0;
}

int DCPCALL ContentGetValue(char* FileName,int FieldIndex,int UnitIndex,void* FieldValue,int maxlen,int flags)
{
		int ret1,ret2;
		char pname[PATH_MAX];
		fprintf(stdout, "%d %s\n", strlen(FileName), FileName);
		struct stat buf, bfparent;
		const char *magic_full;
		magic_t magic_cookie;
		if ((ret1 = lstat(FileName, &buf))!=0)
		{
			fprintf(stderr, "stat failure error .%d", ret1);
			return ft_fileerror;
		}
		struct passwd *pw = getpwuid(buf.st_uid);
		struct group  *gr = getgrgid(buf.st_gid);

		switch (FieldIndex) {
		case 0:
			magic_cookie = magic_open(MAGIC_NONE);
			break;
		case 1:
			if (flags && CONTENT_DELAYIFSLOW)
				return ft_fieldempty;

			switch(UnitIndex)
			{
			case 0:
				magic_cookie = magic_open(MAGIC_NO_CHECK_SOFT);
				break;
			case 1:
				magic_cookie = magic_open(MAGIC_SYMLINK);
				break;
			case 2:
				magic_cookie = magic_open(MAGIC_COMPRESS);;
				break;
			case 3:
				magic_cookie = magic_open(MAGIC_SYMLINK|MAGIC_COMPRESS);
				break;
			case 4:
				magic_cookie = magic_open(MAGIC_CONTINUE|MAGIC_SYMLINK|MAGIC_COMPRESS);
				break;
			default:
				magic_cookie = magic_open(MAGIC_NONE);
			}
			break;
		case 2:
			magic_cookie = magic_open(MAGIC_MIME_TYPE);
			break;
		case 3:
			magic_cookie = magic_open(MAGIC_MIME_ENCODING);
			break;

		case 4:
			if (flags && CONTENT_DELAYIFSLOW)
				return ft_fieldempty;

			switch(UnitIndex)
			{
				case 0:
					if (S_ISREG(buf.st_mode))
					{
						*(int*)FieldValue = 1;
					}
					else
					{
						*(int*)FieldValue = 0;
					}
					break;
				case 1:
					if (S_ISDIR(buf.st_mode))
					{
						*(int*)FieldValue = 1;
					}
					else
					{
						*(int*)FieldValue = 0;
					}
					break;
				case 2:
					if (S_ISCHR(buf.st_mode))
					{
						*(int*)FieldValue = 1;
					}
					else
					{
						*(int*)FieldValue = 0;
					}
					break;
				case 3:
					if (S_ISBLK(buf.st_mode))
					{
						*(int*)FieldValue = 1;
					}
					else
					{
						*(int*)FieldValue = 0;
					}
					break;
				case 4:
					if (S_ISFIFO(buf.st_mode))
					{
						*(int*)FieldValue = 1;
					}
					else
					{
						*(int*)FieldValue = 0;
					}
					break;
				case 5:
					if (S_ISLNK(buf.st_mode))
					{
						*(int*)FieldValue = 1;
					}
					else
					{
						*(int*)FieldValue = 0;
					}
					break;
				case 6:
					if (S_ISSOCK(buf.st_mode))
					{
						*(int*)FieldValue = 1;
					}
					else
					{
						*(int*)FieldValue = 0;
					}
					break;
				default:
						*(int*)FieldValue = 0;
					}
			break;
		case 5:
			*(int*)FieldValue=convertDecimalToOctal(buf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
			break;
		case 6:
			strncpy((char*)FieldValue,pw->pw_name,maxlen-1);
			break;
		case 7:
			*(int*)FieldValue=buf.st_uid;
			break;
		case 8:
			strncpy((char*)FieldValue,gr->gr_name,maxlen-1);
			break;
		case 9:
			*(int*)FieldValue=buf.st_gid;
			break;
		case 10:
			*(int*)FieldValue=buf.st_ino;
			break;
		case 11:
			*(int*)FieldValue=buf.st_blksize;
			break;
		case 12:
			*(int*)FieldValue=buf.st_blocks;
			break;
		case 13:
			*(int*)FieldValue=buf.st_nlink;
			break;
		case 14:
			if (S_ISDIR(buf.st_mode))
			{
				sprintf (pname, "%s/..", FileName);
				if ((ret1 = lstat(pname, &bfparent))!=0)
				{
					fprintf(stderr, "stat failure error .%d", ret1);
					return ft_nosuchfield;
				}
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

		if ((FieldIndex >= 0) && (FieldIndex <4))
		{
			if (magic_cookie == NULL) 
			{
				printf("unable to initialize magic library\n");
				return ft_fileerror;
			}
			if (magic_load(magic_cookie, NULL) != 0) {
				printf("cannot load magic database - %s\n", magic_error(magic_cookie));
				magic_close(magic_cookie);
				return ft_fileerror;
			}
			magic_full = magic_file(magic_cookie, FileName);
			strncpy((char*)FieldValue,magic_full,maxlen-1);
			magic_close(magic_cookie);
		}
		return fields[FieldIndex].type;

}
