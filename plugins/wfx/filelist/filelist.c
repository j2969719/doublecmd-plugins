#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <sys/stat.h>

#include <gtk/gtk.h>

#include <string.h>
#include "wfxplugin.h"

#define inFile "/tmp/test.lst"
#define tmpFile "/tmp/test.lst.new"


int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
FILE *fp;

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

int DCPCALL FsInit(int PluginNr,tProgressProc pProgressProc, tLogProc pLogProc,tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;
	return 0;

}

HANDLE DCPCALL FsFindFirst(char* Path,WIN32_FIND_DATAA *FindData)
{
	fp = fopen(inFile, "r");

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
	strlcpy(FindData->cFileName, "<Paste from Clipboard>", PATH_MAX);
	return (HANDLE)(1985);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl,WIN32_FIND_DATAA *FindData)
{
	if (!fp)
		return false;

	char * line = NULL;
	char tname[PATH_MAX];
	size_t len = 0;
	ssize_t read;
	struct stat buf;
	bool found = false;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
	while ((read = getline(&line, &len, fp)) != -1)
	{
		if (PATH_MAX > strlen(line))
			strlcpy(tname, line, strlen(line)-1);
		else
			strlcpy(tname, line, PATH_MAX);
		if (stat(tname, &buf)==0)
		{
			found = true;
			break;
		}
	}
	if (found)
	{
		FindData->nFileSizeLow = buf.st_size;
		strlcpy(FindData->cFileName, tname, PATH_MAX);
		FindData->dwFileAttributes = 0;
		return true;
	}
	return false;
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	if (fp)
		fclose(fp);
	return 0;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return true;

}

BOOL DCPCALL FsMkDir(char* Path)
{
	return true;
}


BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	FILE *ifp, *ofp;
	char * line = NULL;
	char tname[PATH_MAX+1];
	size_t len = 0;

	ifp = fopen(inFile, "r");
	if (ifp)
	{
		ofp = fopen(tmpFile, "w");
		if (ofp)
		{
			while (getline(&line, &len, ifp) != -1)
			{
				strlcpy(line, line, PATH_MAX);
				sprintf(tname, "/%s", line);
				strlcpy(tname, tname, strlen(tname)-1);
				if (strcmp(tname, RemoteName) != 0)
				{
					fprintf(ofp, line);
				}
			}
			fclose(ofp);
		}
		fclose(ifp);
	}
	rename(tmpFile, inFile);
	return true;
}

int DCPCALL FsPutFile(char* LocalName,char* RemoteName,int CopyFlags)
{
	FILE *tfp;
	char * line = NULL;
	char tname[PATH_MAX];
	size_t len = 0;

	tfp = fopen(inFile, "r");
	if (tfp)
	{
		while (getline(&line, &len, tfp) != -1)
		{
			if (PATH_MAX > strlen(line))
				strlcpy(tname, line, strlen(line)-1);
			else
				strlcpy(tname, line, PATH_MAX);
			if (strcmp(tname, LocalName) == 0)
			{
				fclose(tfp);
				return FS_FILE_OK;
			}
		}
		fclose(tfp);
	}
	tfp = fopen(inFile, "a");
	fprintf(tfp, "%s\n", LocalName);
	fclose(tfp);
	return FS_FILE_OK;
}

int DCPCALL FsExecuteFile(HWND MainWin,char* RemoteName,char* Verb)
{
	if (strcmp(Verb, "open") == 0)
	{
		if (strcmp(RemoteName, "/<Paste from Clipboard>") == 0)
		{
			gchar *clb = gtk_clipboard_wait_for_text (gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
			if (!clb)
				return FS_EXEC_ERROR;
			FILE *tfp;
			tfp = fopen(inFile, "a");
			fprintf(tfp, "%s\n", clb);
			fclose(tfp);
			return FS_EXEC_OK;
		}
		else
		{
			char cmd[PATH_MAX+13];

			sprintf(cmd, "xdg-open \"%s\"", RemoteName);
			system(cmd);

			return FS_EXEC_OK;
		}
	}
	return FS_EXEC_ERROR;
}
/*
BOOL DCPCALL FsContentGetDefaultView(char* ViewContents,char* ViewHeaders,char* ViewWidths,char* ViewOptions,int maxlen)
{
	strlcpy(ViewContents, "[=<fs>.link]\\n[=fs.size]", maxlen);
	strlcpy(ViewHeaders, "Link\\nSize", maxlen);
	strlcpy(ViewWidths, "148,23", maxlen);
	strlcpy(ViewOptions, "-1|-1", maxlen);
	return true;
}

int DCPCALL FsContentGetSupportedField(int FieldIndex,char* FieldName,char* Units,int maxlen)
{
	if (FieldIndex == 0)
	{
		strlcpy(FieldName,"link",maxlen-1);
		strlcpy(Units,"",maxlen-1);
		return ft_string;
	}
	else if (FieldIndex == 1)
	{
		strlcpy(FieldName,"size",maxlen-1);
		strlcpy(Units,"",maxlen-1);
		return ft_numeric_64;
	}
	return ft_nomorefields;
}

int DCPCALL FsContentGetValue(char* FileName,int FieldIndex,int UnitIndex,void* FieldValue,int maxlen,int flags)
{
	struct stat buf;
	switch (FieldIndex) {
	case 0:
			strncpy((char*)FieldValue,FileName,maxlen-1);
			return ft_string;
			break;
	case 1:
			if (stat(FileName, &buf)==0)
				*(int*)FieldValue=buf.st_size;
			return ft_numeric_64;
			break;
	default:
			return ft_nosuchfield;
	}
}
*/
