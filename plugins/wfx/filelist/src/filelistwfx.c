#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>
#include <time.h>
#include <utime.h>
#include <string.h>
#include "wfxplugin.h"

#define _plugname "FileList"
#define _filename "filelist.txt"
#define _inFile "/tmp/doublecmd-filelist.txt"
#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))

typedef struct sField
{
	char *name;
	int type;
	char *unit;
} tField;

#define fieldcount (sizeof(fields)/sizeof(tField))

tField fields[] =
{
	{"Basename",		ft_string,	""},
	{"BasenameNoExt",	ft_string,	""},
	{"Dirname",		ft_string,	""},
};

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;

char inFile[PATH_MAX];

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

void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	int64_t ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

unsigned long FileTimeToUnixTime(LPFILETIME ft)
{
	int64_t ll = ft->dwHighDateTime;
	ll = (ll << 32) | ft->dwLowDateTime;
	ll = (ll - 116444736000000000) / 10000000;
	return (unsigned long)ll;
}

static char *basenamenoext(char *file)
{
	char *str = basename(file);
	char *result = strdup(str);
	char *dot = strrchr(result, '.');

	if (dot)
	{
		int offset = dot - result;
		result[offset] = '\0';

		if (result[0] == '\0')
		{
			free(result);
			result = strdup(str);
		}
	}

	return result;
}

bool getFileFromList(FILE *List, WIN32_FIND_DATAA *FindData)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	struct stat buf;
	bool found = false;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while (!found && (read = getline(&line, &len, List)) != -1)
	{
		if (line[read - 1] == '\n')
			line[read - 1] = '\0';

		if ((stat(line, &buf) == 0) && S_ISREG(buf.st_mode))
			found = true;
	}

	if (found)
	{
		FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
		FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;
		strlcpy(FindData->cFileName, line, MAX_PATH - 1);
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;

		FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
		FindData->dwReserved0 = buf.st_mode;

		UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
		UnixTimeToFileTime(buf.st_atime, &FindData->ftLastAccessTime);
	}

	free(line);

	return found;
}

bool inFileList(char *filename)
{
	FILE *tfp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	bool found = false;

	tfp = fopen(inFile, "r");

	if (tfp)
	{
		while (!found && (read = getline(&line, &len, tfp)) != -1)
		{
			if (line[read - 1] == '\n')
				line[read - 1] = '\0';

			if (strcmp(line, filename) == 0)
				found = true;
		}

		free(line);
		fclose(tfp);
	}

	return found;
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;

	return 0;

}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{

	FILE *fp = fopen(inFile, "r");

	if ((fp != NULL) && (getFileFromList(fp, FindData)))
		return (HANDLE)fp;

	return (HANDLE)(-1);
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	FILE *fp = (FILE*)Hdl;

	return getFileFromList(fp, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	FILE *fp = (FILE*)Hdl;

	fclose(fp);

	return 0;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	FILE *ifp, *ofp;
	char *line = NULL;
	char tname[PATH_MAX];
	char tmpFile[PATH_MAX];
	size_t len = 0;

	ifp = fopen(inFile, "r");

	if (ifp)
	{
		snprintf(tmpFile, PATH_MAX, "%s.tmp", inFile);
		ofp = fopen(tmpFile, "w");

		if (ofp)
		{
			while (getline(&line, &len, ifp) != -1)
			{
				snprintf(tname, PATH_MAX, "/%s", line);

				if (tname[strlen(tname) - 1] == '\n')
					tname[strlen(tname) - 1] = '\0';

				if (strcmp(tname, RemoteName) != 0)
					fprintf(ofp, line);

			}

			free(line);
			fclose(ofp);
		}

		fclose(ifp);
	}

	rename(tmpFile, inFile);

	return true;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return true;

}

BOOL DCPCALL FsMkDir(char* Path)
{
	return true;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);

	if (err)
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && inFileList(LocalName))
	{
		gProgressProc(gPluginNr, RemoteName, LocalName, 100);
		return FS_FILE_OK;
	}

	FILE *tfp;

	tfp = fopen(inFile, "a");

	if (tfp)
	{
		fprintf(tfp, "%s\n", LocalName);
		fclose(tfp);
		gProgressProc(gPluginNr, RemoteName, LocalName, 100);
		return FS_FILE_OK;
	}

	return FS_FILE_WRITEERROR;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int ifd, ofd, done;
	ssize_t len, total = 0;
	char rpath[PATH_MAX];
	char dpath[PATH_MAX];
	char buff[8192];
	struct stat buf;
	struct utimbuf ubuf;
	int result = FS_FILE_OK;

	strlcpy(rpath, RemoteName + 1, PATH_MAX);

	if (strlen(LocalName) > strlen(rpath) && strstr(LocalName, RemoteName))
	{
		strlcpy(dpath, LocalName, strlen(LocalName) - strlen(rpath));
		strncat(dpath, basename(LocalName), PATH_MAX);
	}
	else
		strlcpy(dpath, LocalName, PATH_MAX);


	if (strcmp(dpath, rpath) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (CopyFlags == 0 && access(dpath, F_OK) == 0)
		return FS_FILE_EXISTS;

	if (gProgressProc(gPluginNr, rpath, dpath, 0) == 1)
		return FS_FILE_USERABORT;

	ifd = open(rpath, O_RDONLY);

	if (ifd == -1)
		return FS_FILE_READERROR;

	ofd = open(dpath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (ofd > -1)
	{
		size_t rsize = (size_t)((int64_t)ri->SizeHigh << 32 | ri->SizeLow);

		while ((len = read(ifd, buff, sizeof(buff))) > 0)
		{
			if (write(ofd, buff, len) == -1)
			{
				result = FS_FILE_WRITEERROR;
				break;
			}

			total += len;

			if (rsize > 0)
				done = total * 100 / rsize;
			else
				done = 0;

			if (done > 100)
				done = 100;

			if (gProgressProc(gPluginNr, rpath, dpath, done) == 1)
			{
				result = FS_FILE_USERABORT;
				break;
			}
		}

		close(ofd);

		if (ri->Attr > 0)
			chmod(dpath, ri->Attr);

		if (stat(rpath, &buf) == 0)
		{
			ubuf.actime = buf.st_atime;
			ubuf.modtime = buf.st_mtime;
			utime(dpath, &ubuf);
		}

	}
	else
		result = FS_FILE_WRITEERROR;

	close(ifd);

	return result;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strcmp(Verb, "open") == 0)
	{
//		return FS_EXEC_YOURSELF;

		struct stat buf;
		char *command = malloc(strlen(RemoteName) + 12);

		if (stat(RemoteName + 1, &buf) == 0)
		{
			if (buf.st_mode & S_IXUSR)
				sprintf(command, "\"%s\"", RemoteName + 1);
			else
				sprintf(command, "xdg-open \"%s\"", RemoteName + 1);

			system(command);
			free(command);
		}
		else
			return FS_EXEC_ERROR;
	}
	else if (strncmp(Verb, "chmod", 5) == 0)
	{
		int mode = strtoll(Verb + 6, 0, 8);

		if (chmod(RemoteName + 1, mode) == -1)
			return FS_EXEC_ERROR;
	}
	else if (gRequestProc)
		gRequestProc(gPluginNr, RT_MsgOK, NULL, strerror(EOPNOTSUPP), NULL, 0);

	return FS_EXEC_OK;
}

BOOL DCPCALL FsSetTime(char* RemoteName, FILETIME *CreationTime, FILETIME *LastAccessTime, FILETIME *LastWriteTime)
{
	struct stat buf;
	struct utimbuf ubuf;

	if (LastAccessTime != NULL || LastWriteTime != NULL)
	{

		if (stat(RemoteName + 1, &buf) == 0)
		{
			if (LastAccessTime != NULL)
				ubuf.actime = FileTimeToUnixTime(LastAccessTime);
			else
				ubuf.actime = buf.st_atime;

			if (LastWriteTime != NULL)
				ubuf.modtime = FileTimeToUnixTime(LastWriteTime);
			else
				ubuf.modtime = buf.st_mtime;

			if (utime(RemoteName + 1, &ubuf) == 0)
				return true;
		}
	}

	return false;
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	strlcpy(Units, fields[FieldIndex].unit, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int result = fields[FieldIndex].type;
	char *strvalue = NULL;

	switch (FieldIndex)
	{
	case 0:
		strvalue = basename(FileName);

		if (strvalue)
			strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			result = ft_fieldempty;

		break;

	case 1:
		strvalue = basenamenoext(FileName);

		if (strvalue)
		{
			strlcpy((char*)FieldValue, strvalue, maxlen - 1);
			free(strvalue);
		}
		else
			result = ft_fieldempty;

		break;

	case 2:
		strvalue = dirname(FileName + 1);

		if (strvalue)
			strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			result = ft_fieldempty;

		break;

	default:
		result = ft_nosuchfield;
	}

	return result;
}


void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	strlcpy(inFile, dps->DefaultIniName, PATH_MAX - 1);
	char *pos = strrchr(inFile, '/');

	if (pos)
		strcpy(pos + 1, _filename);
	else
		strlcpy(inFile, _inFile, PATH_MAX);
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	return false;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	strlcpy(DefRootName, _plugname, maxlen - 1);
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	strlcpy(RemoteName, RemoteName + 1, maxlen - 1);
	return true;
}
