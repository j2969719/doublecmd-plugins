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
GKeyFile *gCfg = NULL;
static gchar gCfgPath[PATH_MAX];

int DCPCALL Init(tDsxDefaultParamStruct* dsp, tSAddFileProc pAddFileProc, tSUpdateStatusProc pUpdateStatus)
{
	gAddFileProc = pAddFileProc;
	gUpdateStatus = pUpdateStatus;

	Dl_info dlinfo;
	static char plg_path[PATH_MAX];
	const char* loc_dir = "langs";
	GError *err = NULL;

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

		g_strlcpy(gCfgPath, dlinfo.dli_fname, PATH_MAX);
		pos = strrchr(gCfgPath, '/');

		if (pos)
			strcpy(pos + 1, "settings.ini");

		gCfg = g_key_file_new();
	}

	return 0;
}

void DCPCALL StartSearch(int PluginNr, tDsxSearchRecord* pSearchRec)
{
	gchar *argv[7];
	gchar *line = NULL, *dppath = NULL;
	gchar locate_opts[7] = "-q";
	gsize len, term, i;
	GPid pid;
	gint fp;
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	GError *err = NULL;
	gboolean dirsonly = FALSE, exludedirs = FALSE;
	gboolean startpath;

	i = strlen(locate_opts);

	if (!g_key_file_load_from_file(gCfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("%s: %s\n", gCfgPath, (err)->message);

	if (g_key_file_get_boolean(gCfg, "Search", "Basename", NULL) == TRUE)
		locate_opts[i++] = 'b';

	if (g_key_file_get_boolean(gCfg, "Search", "IgnoreCase", NULL) == TRUE)
		locate_opts[i++] = 'i';

	if (g_key_file_get_boolean(gCfg, "Search", "Existing", NULL) == TRUE)
		locate_opts[i++] = 'e';

	if (g_key_file_get_boolean(gCfg, "Search", "NoFolow", NULL) == TRUE)
		locate_opts[i++] = 'P';

	startpath = g_key_file_get_boolean(gCfg, "Search", "StartPath", NULL);
	dppath = g_key_file_get_string(gCfg, "DB", "Path", NULL);

	i = 0;
	argv[i++] = "locate";
	argv[i++] = locate_opts;

	if (dppath)
	{
		argv[i++] = "-d";
		argv[i++] = dppath;
	}

	if (g_key_file_get_boolean(gCfg, "Search", "RegEx", NULL) == TRUE)
		argv[i++] = "--regex";

	argv[i++] = pSearchRec->FileMask;
	argv[i++] = NULL;

	if (g_strrstr(pSearchRec->AttribStr, "d+") != NULL)
		dirsonly = TRUE;

	if (g_strrstr(pSearchRec->AttribStr, "d-") != NULL)
		exludedirs = TRUE;

	gUpdateStatus(PluginNr, _("not found"), 0);
	gStop = FALSE;
	i = 1;

	if (!g_spawn_async_with_pipes(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL, &fp, NULL, &err))

		gUpdateStatus(PluginNr, err->message, 0);

	else
	{
		GIOChannel *stdout = g_io_channel_unix_new(fp);

		while (!gStop && (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout, &line, &len, &term, NULL)))
		{
			if (line)
			{

				line[term] = '\0';

				if (dirsonly || exludedirs)
				{
					gboolean isdir = g_file_test(line, G_FILE_TEST_IS_DIR);

					if ((dirsonly && isdir) || (exludedirs && !isdir))
					{
						if (startpath)
						{
							if (strncmp(line, pSearchRec->StartPath, strlen(pSearchRec->StartPath) - 1) == 0)
								gAddFileProc(PluginNr, line);
						}
						else
							gAddFileProc(PluginNr, line);
					}
				}
				else if (startpath)
				{
					if (strncmp(line, pSearchRec->StartPath, strlen(pSearchRec->StartPath) - 1) == 0)
						gAddFileProc(PluginNr, line);
				}
				else
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

	if (dppath)
		g_free(dppath);

	if (err)
		g_error_free(err);

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	gStop = TRUE;

	if (gCfg != NULL)
	{
		g_key_file_free(gCfg);
		gCfg = NULL;
	}
}

void DCPCALL Finalize(int PluginNr)
{

}
