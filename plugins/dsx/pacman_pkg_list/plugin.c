#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

#define cmdf "pacman -Qlq %s --noconfirm"

int DCPCALL Init(tDsxDefaultParamStruct* dsp, tSAddFileProc pAddFileProc, tSUpdateStatusProc pUpdateStatus)
{
	gAddFileProc = pAddFileProc;
	gUpdateStatus = pUpdateStatus;

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
	char *str, *line = NULL;
	char pkgname[PATH_MAX + 1];

	strncpy(pkgname, pSearchRec->FileMask, PATH_MAX);

	for (i = 0; i <= strlen(pkgname); i++)
	{
		if (pkgname[i] == '*')
			pkgname[i] = ' ';
	}

	if (strcmp(pkgname, " ") != 0 && strcmp(pkgname, "   ") != 0)
	{
		i = 1;
		asprintf(&str, cmdf, pkgname);
		gUpdateStatus(PluginNr, str, 0);

		if ((fp = popen(str, "r")) == NULL)
			gUpdateStatus(PluginNr, _("failed to launch command"), 0);
		else
		{
			gUpdateStatus(PluginNr, _("not found"), 0);

			while (getline(&line, &len, fp) != -1)
				if (line && line != "")
				{
					line[strlen(line) - 1] = '\0';

					if (line[strlen(line) - 1] == '/')
						line[strlen(line) - 1] = '\0';

					if (access(line, F_OK) != -1)
					{
						gAddFileProc(PluginNr, line);
						gUpdateStatus(PluginNr, line, i++);
					}
				}

			pclose(fp);
			free(str);
		}
	}
	else
		gUpdateStatus(PluginNr, _("enter pkgname"), 0);

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{

}

void DCPCALL Finalize(int PluginNr)
{

}
