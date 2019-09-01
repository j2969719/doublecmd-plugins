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
					gAddFileProc(PluginNr, fname);
					gUpdateStatus(PluginNr, fname, i++);
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
