#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <fnmatch.h>
#include <linux/limits.h>
#include <string.h>
#include "dsxplugin.h"

#include <dlfcn.h>

#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

char inFile[PATH_MAX + 1];
bool stop_search;

int DCPCALL Init(tDsxDefaultParamStruct* dps, tSAddFileProc pAddFileProc, tSUpdateStatusProc pUpdateStatus)
{
	gAddFileProc = pAddFileProc;
	gUpdateStatus = pUpdateStatus;

	strncpy(inFile, dps->DefaultIniName, PATH_MAX);

	char *pos = strrchr(inFile, '/');

	if (pos)
		strcpy(pos + 1, "filelist.txt");
	else
		strncpy(inFile, "/tmp/doublecmd-filelist.txt", PATH_MAX);

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

	return 0;
}

void DCPCALL StartSearch(int PluginNr, tDsxSearchRecord* pSearchRec)
{
	FILE *fp;
	size_t i = 1, len = 0;
	ssize_t read;
	char *line = NULL;
	stop_search = false;

	gUpdateStatus(PluginNr, inFile, 0);

	if ((fp = fopen(inFile, "r")) != NULL)
	{

		gUpdateStatus(PluginNr, _("not found"), 0);

		while (!stop_search && (read = getline(&line, &len, fp)) != -1)
			if (line && line[0] != '\n')
			{
				line[read - 1] = '\0';

				if (access(line, F_OK) != -1)
				{
					if (fnmatch(pSearchRec->FileMask, basename(line), 0) == 0)
					{
						gAddFileProc(PluginNr, line);
						gUpdateStatus(PluginNr, line, i++);
					}
				}
			}

		fclose(fp);
	}

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	stop_search = true;
}

void DCPCALL Finalize(int PluginNr)
{

}
