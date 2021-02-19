#define _GNU_SOURCE
#include <glib.h>
#include <signal.h>
#include <fnmatch.h>
#include "dsxplugin.h"

#include <dlfcn.h>

#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

gboolean gStop;
GKeyFile *gCfg = NULL;


int DCPCALL Init(tDsxDefaultParamStruct* dsp, tSAddFileProc pAddFileProc, tSUpdateStatusProc pUpdateStatus)
{
	gAddFileProc = pAddFileProc;
	gUpdateStatus = pUpdateStatus;

	Dl_info dlinfo;
	static char plg_path[PATH_MAX];
	static char cfg_path[PATH_MAX];
	const char* loc_dir = "langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(plg_path, &dlinfo) != 0)
	{
		g_strlcpy(plg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plg_path, '/');

		if (pos)
			strcpy(pos + 1, loc_dir);

		setlocale(LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, plg_path);
		textdomain(GETTEXT_PACKAGE);

		g_strlcpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, "settings.ini");

		gCfg = g_key_file_new();
		g_key_file_load_from_file(gCfg, cfg_path, 0, NULL);
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
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	GError *err = NULL;
	gboolean found, mask, path;

	if (!pSearchRec->IsFindText)
	{
		gUpdateStatus(PluginNr, _("the search text was not specified"), 0);
		gAddFileProc(PluginNr, "");
		return;
	}

	gStop = FALSE;
	command = g_strdup_printf("recollq -b '%s'", pSearchRec->FindText);
	gUpdateStatus(PluginNr, command, 0);

	if (!g_shell_parse_argv(command, NULL, &argv, &err))
	{
		gUpdateStatus(PluginNr, err->message, 0);
		gAddFileProc(PluginNr, "");
		g_error_free(err);
		return;
	}

	g_clear_error(&err);

	if (!g_spawn_async_with_pipes(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL, &fp, NULL, &err))
	{
		gUpdateStatus(PluginNr, err->message, 0);
		gAddFileProc(PluginNr, "");
	}
	else
	{
		mask = g_key_file_get_boolean(gCfg, "Search", "check_mask", NULL);
		path = g_key_file_get_boolean(gCfg, "Search", "check_path", NULL);

		gUpdateStatus(PluginNr, _("not found"), 0);

		GIOChannel *stdout = g_io_channel_unix_new(fp);

		while (!gStop && (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout, &line, &len, &term, NULL)))
		{
			line[term] = '\0';
			found = false;

			if (strncmp(line, "file:///", 8) == 0)
			{
				gchar *fname = g_filename_from_uri(line, NULL, NULL);

				if (fname)
				{
					gchar *bname = g_path_get_basename(fname);

					if (!mask && !path)
						found = true;
					else if (mask && !path)
					{
						if (fnmatch(pSearchRec->FileMask, bname, 0) == 0)
							found = true;
					}
					else if (!mask && path)
					{
						if (strncmp(pSearchRec->StartPath, fname, strlen(pSearchRec->StartPath)) == 0)
							found = true;
					}
					else
					{
						if (strncmp(pSearchRec->StartPath, fname, strlen(pSearchRec->StartPath)) == 0 && fnmatch(pSearchRec->FileMask, bname, 0) == 0)
							found = true;
					}

					if (found)
					{
						gAddFileProc(PluginNr, fname);
						gUpdateStatus(PluginNr, fname, i++);
					}

					g_free(bname);
					g_free(fname);
				}
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
	gStop = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{
	if (gCfg != NULL)
	{
		g_key_file_free(gCfg);
		gCfg = NULL;
	}
}
