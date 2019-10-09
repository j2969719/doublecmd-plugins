#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <fnmatch.h>
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
	char *line = NULL;

	gUpdateStatus(PluginNr, "lslocks -o PATH -u -n", 0);

	if ((fp = popen("lslocks -o PATH -u -n", "r")) == NULL)
		gUpdateStatus(PluginNr, "failed to launch command", 0);
	else
	{
		gUpdateStatus(PluginNr, "not found", 0);

		while (getline(&line, &len, fp) != -1)
			if (line && line != "")
			{
				line[strlen(line) - 1] = '\0';

				if (fnmatch(pSearchRec->FileMask, basename(line), 0) == 0)
				{
					gAddFileProc(PluginNr, line);
					gUpdateStatus(PluginNr, line, i++);
				}
			}

		pclose(fp);
	}

	if (line != NULL)
		free(line);

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{

}

void DCPCALL Finalize(int PluginNr)
{

}
