#include <glib.h>
#include <signal.h>
#include "dsxplugin.h"

#define cmdf "/bin/sh -c \"find '%s' -type f -name '%s' -exec sha1sum '{}' ';' | sort | uniq --all-repeated=separate -w 40 | cut -c 43-\""


tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

gboolean stop_search;
GPid pid;

int DCPCALL Init(tDsxDefaultParamStruct* dsp, tSAddFileProc pAddFileProc, tSUpdateStatusProc pUpdateStatus)
{
	gAddFileProc = pAddFileProc;
	gUpdateStatus = pUpdateStatus;
	return 0;
}

void DCPCALL StartSearch(int PluginNr, tDsxSearchRecord* pSearchRec)
{
	gchar **argv;
	gchar *line, *command;
	gsize len, term, i = 1;

	gint fp;
	GSpawnFlags flags;
	GError *err = NULL;

	stop_search = FALSE;
	command = g_strdup_printf(cmdf, pSearchRec->StartPath, pSearchRec->FileMask);
	gUpdateStatus(PluginNr, command, 0);

	if (!g_shell_parse_argv(command, NULL, &argv, &err))
	{
		gUpdateStatus(PluginNr, g_strdup(err->message), 0);
		gAddFileProc(PluginNr, "");
	}

	g_clear_error(&err);

	if (!g_spawn_async_with_pipes(NULL, argv, NULL, 0, NULL, NULL, &pid, NULL, &fp, NULL, &err))
	{
		gUpdateStatus(PluginNr, g_strdup(err->message), 0);
		gAddFileProc(PluginNr, "");
	}
	else
	{

		GIOChannel *stdout = g_io_channel_unix_new(fp);

		while (!stop_search && (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout, &line, &len, &term, NULL)))
		{
			line[term] = '\0';

			if (line[0] == '\0')
				gAddFileProc(PluginNr, "-----------------------------------------");
			else
			{
				gAddFileProc(PluginNr, line);
				gUpdateStatus(PluginNr, line, i++);
			}
		}

		kill(pid, SIGTERM);
		g_spawn_close_pid(pid);
		g_io_channel_shutdown(stdout, TRUE, NULL);
		g_io_channel_unref(stdout);
	}

	if (err)
		g_error_free(err);

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	stop_search = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{

}
