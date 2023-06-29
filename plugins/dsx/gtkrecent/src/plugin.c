#define _GNU_SOURCE
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
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
	GList *list;
	gsize i = 1;
	GPatternSpec *pattern;
	GtkRecentManager *manager;
	gStop = FALSE;

	gUpdateStatus(PluginNr, _("not found"), 0);
	manager = gtk_recent_manager_get_default();
	list = gtk_recent_manager_get_items(manager);
	pattern = g_pattern_spec_new(pSearchRec->FileMask);

	while (!gStop && list != NULL)
	{
		const gchar *uri = gtk_recent_info_get_uri(list->data);

		if (uri)
		{
			gchar *fname = g_filename_from_uri(uri, NULL, NULL);

			if (fname) // && strncmp(fname, pSearchRec->StartPath, strlen(pSearchRec->StartPath)) == 0)
			{
				gchar *bname = g_path_get_basename(fname);

				if (g_pattern_spec_match_string(pattern, bname))
				{
					if (pSearchRec->IsFindText)
					{
						gchar *contents;
						gchar *haystack;
						gchar *needle;

						g_file_get_contents(fname, &contents, NULL, NULL);

						if (contents != NULL)
						{
							if (pSearchRec->CaseSensitive)
							{
								haystack = g_strdup(contents);
								needle = g_strdup(pSearchRec->FindText);
							}
							else
							{
								haystack = g_utf8_strdown(contents, -1);
								needle = g_utf8_strdown(pSearchRec->FindText, -1);
							}

							g_free(contents);

							if (g_strrstr(haystack, needle) != NULL)
							{
								gAddFileProc(PluginNr, fname);
								gUpdateStatus(PluginNr, fname, i++);
							}

							g_free(needle);
							g_free(haystack);
						}
					}
					else
					{
						gAddFileProc(PluginNr, fname);
						gUpdateStatus(PluginNr, fname, i++);
					}
				}

				g_free(bname);
				g_free(fname);
			}
		}

		gtk_recent_info_unref(list->data);
		list = list->next;
	}

	g_list_free(list);
	g_pattern_spec_free(pattern);

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	gStop = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{

}
