#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>
#include "dsxplugin.h"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

#define cmdf "find '%s' -type f -name '%s' -exec sha1sum '{}' ';' | sort | uniq --all-repeated=separate -w 40 | cut -c 43-"

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

	asprintf(&str, cmdf, pSearchRec->StartPath, pSearchRec->FileMask);
	gUpdateStatus(PluginNr, str, 0);

	if ((fp = popen(str, "r")) == NULL)
		gAddFileProc(PluginNr, "");

	while(getline(&line, &len, fp) != -1)
		if (line && line != "")
		{
			line[strlen(line)-1] = '\0';
			if (strcmp(line, "") == 0)
				gAddFileProc(PluginNr, "-----------------------------------------");
			else
			{
				gAddFileProc(PluginNr, line);
				gUpdateStatus(PluginNr, line, i++);
			}
		}

	pclose(fp);
	free(str);
	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{

}

void DCPCALL Finalize(int PluginNr)
{

}
