#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>
#include "dsxplugin.h"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

int DCPCALL Init(tDsxDefaultParamStruct* dsp, tSAddFileProc pAddFileProc, tSUpdateStatusProc pUpdateStatus)
{
	gAddFileProc = pAddFileProc;
	gUpdateStatus = pUpdateStatus;
	return 0;
}

void DCPCALL StartSearch(int PluginNr, tDsxSearchRecord* pSearchRec)
{
	FILE *fp;
	size_t i = 1, len = 0;
	char *str, *line = NULL;

	asprintf(&str, "cd '%s' && git ls-files -o '%s'", pSearchRec->StartPath, pSearchRec->FileMask);
	gUpdateStatus(PluginNr, str, 0);

	if ((fp = popen(str, "r")) == NULL)
		gUpdateStatus(PluginNr, "failed to launch command", 0);
	else
	{
		gUpdateStatus(PluginNr, "not found", 0);

		while (getline(&line, &len, fp) != -1)
			if (line && line != "")
			{
				line[strlen(line) - 1] = '\0';
				asprintf(&str, "%s%s", pSearchRec->StartPath, line);

				if (access(str, F_OK) != -1)
				{
					gAddFileProc(PluginNr, str);
					gUpdateStatus(PluginNr, str, i++);
				}
			}

		pclose(fp);
		free(str);
	}

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{

}

void DCPCALL Finalize(int PluginNr)
{

}
