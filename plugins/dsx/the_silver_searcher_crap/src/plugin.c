#define _GNU_SOURCE
#include <glib.h>
#include <signal.h>
#include "dsxplugin.h"

#include <dlfcn.h>

#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

gboolean gStop;

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
	gchar **argv = NULL;
	gchar *line = NULL, *command = NULL;
	gsize len, term, i = 1;
	GPid pid;
	gint fp;
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	GError *err = NULL;

	gStop = FALSE;

	if (system("ag --list-file-types") == 0)
	{
		if (pSearchRec->IsFindText && pSearchRec->FindText[0] != '\0')
		{
			if (pSearchRec->FileMask[0] != '\0' && g_strcmp0(pSearchRec->FileMask, "*") != 0 && g_strcmp0(pSearchRec->FileMask, "***") != 0)

				command = g_strdup_printf("ag '%s' -l --nocolor --silent -G '%s' '%s'", pSearchRec->FindText, pSearchRec->FileMask, pSearchRec->StartPath);
			else
				command = g_strdup_printf("ag '%s' -l --nocolor --silent '%s'", pSearchRec->FindText, pSearchRec->StartPath);

			if (pSearchRec->CaseSensitive)
				command = g_strdup_printf("%s -s", command);

			gUpdateStatus(PluginNr, command, 0);

			if (!g_shell_parse_argv(command, NULL, &argv, &err))
				gUpdateStatus(PluginNr, err->message, 0);
			else
			{
				g_clear_error(&err);

				if (g_spawn_async_with_pipes(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL, &fp, NULL, &err))
				{
					gUpdateStatus(PluginNr, _("not found"), 0);
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
				}
				else
					gUpdateStatus(PluginNr, err->message, 0);
			}
		}
		else
			gUpdateStatus(PluginNr, _("no text pattern provided"), 0);
	}
	else
		gUpdateStatus(PluginNr, _("failed to launch ag..."), 0);

	if (command)
		g_free(command);

	if (argv)
		g_strfreev(argv);

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
