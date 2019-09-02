#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
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
	GList *list;
	gsize i = 1;
	GPatternSpec *pattern;
	GtkRecentManager *manager;
	stop_search = FALSE;

	gUpdateStatus(PluginNr, "not found", 0);
	manager = gtk_recent_manager_get_default();
	list = gtk_recent_manager_get_items(manager);
	pattern = g_pattern_spec_new(pSearchRec->FileMask);

	while (!stop_search && list != NULL)
	{
		const gchar *uri = gtk_recent_info_get_uri(list->data);

		if (uri)
		{
			gchar *fname = g_filename_from_uri(uri, NULL, NULL);

			if (fname) // && strncmp(fname, pSearchRec->StartPath, strlen(pSearchRec->StartPath)) == 0)
			{
				if (g_pattern_match_string(pattern, g_path_get_basename(fname)))
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
	stop_search = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{

}
