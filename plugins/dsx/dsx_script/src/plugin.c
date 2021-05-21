#define _GNU_SOURCE
#include <glib.h>
#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>
#include "dsxplugin.h"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;
static char gScript[PATH_MAX];
gboolean gStop;

int DCPCALL Init(tDsxDefaultParamStruct* dsp, tSAddFileProc pAddFileProc, tSUpdateStatusProc pUpdateStatus)
{
	Dl_info dlinfo;
	const char* script_file = "script.sh";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(script_file, &dlinfo) != 0)
	{
		strncpy(gScript, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(gScript, '/');

		if (pos)
			strcpy(pos + 1, script_file);
	}

	gAddFileProc = pAddFileProc;
	gUpdateStatus = pUpdateStatus;
	return 0;
}

void DCPCALL StartSearch(int PluginNr, tDsxSearchRecord* pSearchRec)
{
	gchar *line;
	gsize len, term, i = 1;
	GPid pid;
	gint fp;
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	GError *err = NULL;

	gchar *argv[] = {
		gScript,
		pSearchRec->StartPath,
		pSearchRec->FileMask,
		pSearchRec->FindText,
		pSearchRec->ReplaceText,
		NULL
	};

	gStop = FALSE;
	gUpdateStatus(PluginNr, "not found", 0);

	if (!g_spawn_async_with_pipes(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL, &fp, NULL, &err))
	{
		gUpdateStatus(PluginNr, err->message, 0);
		gAddFileProc(PluginNr, "");
	}
	else
	{

		GIOChannel *stdout = g_io_channel_unix_new(fp);

		while (!gStop && (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout, &line, &len, &term, NULL)))
		{
			if (line)
			{
				line[term] = '\0';
				gAddFileProc(PluginNr, line);
				gUpdateStatus(PluginNr, line, i++);
				g_free(line);
			}
		}

		kill(pid, SIGTERM);
		g_spawn_close_pid(pid);
		g_io_channel_shutdown(stdout, TRUE, NULL);
		g_io_channel_unref(stdout);
		close(fp);
	}

	if (err)
		g_error_free(err);

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	gStop = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{

}
