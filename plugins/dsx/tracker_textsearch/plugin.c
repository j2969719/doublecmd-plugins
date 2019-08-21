#include <glib.h>
#include <libtracker-sparql/tracker-sparql.h>
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
	GError *error = NULL;
	TrackerSparqlConnection *connection;
	TrackerSparqlCursor *cursor;
	gsize len, term, i = 1;
	gUpdateStatus(PluginNr, "...", 0);

	if (pSearchRec->IsFindText && strlen(pSearchRec->FindText) > 3)
	{
		gchar *query = "SELECT nie:url(?f) WHERE { ?f fts:match '%s' }";
		query = g_strdup_printf(query, pSearchRec->FindText);
		connection = tracker_sparql_connection_get(NULL, &error);

		if (connection)
		{
			cursor = tracker_sparql_connection_query(connection, query, NULL, &error);

			if (cursor)
			{
				while (tracker_sparql_cursor_next(cursor, NULL, NULL))
				{
					const gchar *uri = tracker_sparql_cursor_get_string(cursor, 0, NULL);

					if (uri)
					{
						//g_print("%s\n", uri);
						gchar *fname = g_filename_from_uri(uri, NULL, NULL);

						if (fname)
						{
							gAddFileProc(PluginNr, fname);
							gUpdateStatus(PluginNr, fname, i++);
						}
					}
				}
			}
		}


		if (error)
		{
			gUpdateStatus(PluginNr, error->message, 0);
			g_clear_error(&error);

		}

		if (connection)
			g_object_unref(connection);
	}
	else
		gUpdateStatus(PluginNr, "no text provided", 0);

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	stop_search = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{

}
