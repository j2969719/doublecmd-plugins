#include <glib.h>
#include <signal.h>
#include "dsxplugin.h"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

gboolean stop_search;

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
	GPid pid;
	gint fp;
	GSpawnFlags flags;
	GError *err = NULL;

	stop_search = FALSE;

	if (system("locate -V") == 0)
	{
		command = g_strdup_printf("locate -e -i -b '%s'", pSearchRec->FileMask);
		gUpdateStatus(PluginNr, command, 0);



		if (!g_shell_parse_argv(command, NULL, &argv, &err))
		{
			gUpdateStatus(PluginNr, g_strdup(err->message), 0);
			gAddFileProc(PluginNr, "");
		}

		flags |= G_SPAWN_SEARCH_PATH ;

		if (!g_spawn_async_with_pipes(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL, &fp, NULL, &err))
		{
			gAddFileProc(PluginNr, "");
		}

		GIOChannel *stdout = g_io_channel_unix_new(fp);

		while (!stop_search && (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout, &line, &len, &term, NULL)))
		{
			line[term] = '\0';
			gAddFileProc(PluginNr, line);
			gUpdateStatus(PluginNr, line, i++);
		}

		kill(pid, SIGTERM);
		g_spawn_close_pid(pid);
		g_io_channel_shutdown(stdout, TRUE, NULL);
		g_io_channel_unref(stdout);
	}
	else
		gUpdateStatus(PluginNr, "Failed to launch locate...", 0);

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	stop_search = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{

}
