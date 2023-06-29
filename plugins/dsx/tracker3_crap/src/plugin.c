#define _GNU_SOURCE
#include <glib.h>
#include <signal.h>
#include <unistd.h>
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
	}

	char cfg_path[PATH_MAX];
	g_strlcpy(cfg_path, dsp->DefaultIniName, PATH_MAX);
	char *pos = strrchr(cfg_path, '/');

	if (pos)
		strcpy(pos + 1, "j2969719_dsx.ini");

	gCfg = g_key_file_new();

	if (!g_key_file_load_from_file(gCfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL) || !g_key_file_has_group(gCfg, PLUGNAME))
	{
		g_key_file_set_boolean(gCfg, PLUGNAME, "CheckMask", FALSE);
		g_key_file_set_boolean(gCfg, PLUGNAME, "CheckPath", FALSE);
		g_key_file_save_to_file(gCfg, cfg_path, NULL);
	}

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
	gboolean found, mask, path;

	if (!pSearchRec->IsFindText)
	{
		gUpdateStatus(PluginNr, _("the search text was not specified"), 0);
		gAddFileProc(PluginNr, "");
		return;
	}

	gStop = FALSE;

	gchar *argv[] =
	{
		"tracker3",
		"search",
		"--disable-color",
		pSearchRec->FindText,
		NULL
	};

	if (!g_spawn_async_with_pipes(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL, &fp, NULL, &err))
	{
		gUpdateStatus(PluginNr, err->message, 0);
		gAddFileProc(PluginNr, "");
	}
	else
	{
		mask = g_key_file_get_boolean(gCfg, PLUGNAME, "CheckMask", NULL);
		path = g_key_file_get_boolean(gCfg, PLUGNAME, "CheckPath", NULL);

		gUpdateStatus(PluginNr, _("not found"), 0);

		GIOChannel *stdout = g_io_channel_unix_new(fp);

		while (!gStop && (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout, &line, &len, &term, NULL)))
		{
			line[term] = '\0';
			found = false;
			g_strchug(line);

			if (g_str_has_prefix(line, "file:///"))
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
	if (gCfg != NULL)
	{
		g_key_file_free(gCfg);
		gCfg = NULL;
	}
}
