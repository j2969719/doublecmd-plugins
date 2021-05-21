#define _GNU_SOURCE
#include <glib.h>
#include <signal.h>
#include <unistd.h>
#include "dsxplugin.h"

#include <dlfcn.h>

#include <libintl.h>
#include <locale.h>

#define cmdf "/bin/sh -c \"find '%s' -links +1 -type f -name '%s' -printf '%%20i\\t%%p\\n' | sort | uniq --all-repeated=separate -w 20 | cut -f2-\""
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

gboolean stop_search;

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

		setlocale(LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, plg_path);
		textdomain(GETTEXT_PACKAGE);
	}

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
	command = g_strdup_printf(cmdf, pSearchRec->StartPath, pSearchRec->FileMask);
	gUpdateStatus(PluginNr, command, 0);

	if (!g_shell_parse_argv(command, NULL, &argv, &err))
	{
		gUpdateStatus(PluginNr, err->message, 0);
		gAddFileProc(PluginNr, "");
	}

	g_clear_error(&err);

	if (!g_spawn_async_with_pipes(NULL, argv, NULL, 0, NULL, NULL, &pid, NULL, &fp, NULL, &err))
	{
		gUpdateStatus(PluginNr, err->message, 0);
		gAddFileProc(PluginNr, "");
	}
	else
	{

		gUpdateStatus(PluginNr, _("not found"), 0);

		GIOChannel *stdout = g_io_channel_unix_new(fp);

		while (!stop_search && (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout, &line, &len, &term, NULL)))
		{
			if (line)
			{
				line[term] = '\0';

				if (line[0] == '\0')
					gAddFileProc(PluginNr, "-----------------------------------------");
				else
				{
					gAddFileProc(PluginNr, line);
					gUpdateStatus(PluginNr, line, i++);
				}

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
	stop_search = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{

}
