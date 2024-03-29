#define _GNU_SOURCE
#include <glib.h>
#include <libtracker-sparql/tracker-sparql.h>
#include "dsxplugin.h"

#include <dlfcn.h>

#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#define REMOTE_NAME "org.freedesktop.Tracker3.Miner.Files"

#define NOTEXT_QUERYF "\
SELECT DISTINCT nie:url(?s) FROM tracker:FileSystem \
WHERE \
{\
 ?s a nfo:FileDataObject .\
 ?s nie:url ?url . \
 FILTER (STRSTARTS (?s, '%s')) . \
 {\
   ?s nfo:fileName ?name .\
   FILTER (CONTAINS(LCASE(?name), LCASE('%s'))) \
 }\
}"
#define TEXT_QUERYF "\
SELECT DISTINCT nie:url(?s) FROM tracker:FileSystem \
FROM tracker:Documents FROM tracker:Audio FROM tracker:Video \
WHERE \
{\
 ?s a nfo:FileDataObject .\
 ?s nie:url ?url . \
 FILTER (STRSTARTS (?s, '%s')) . \
 ?c nie:isStoredAs ?s. ?c fts:match '%s' . \
 {\
   ?s nfo:fileName ?name .\
   FILTER (CONTAINS(LCASE(?name), LCASE('%s'))) \
 }\
}"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

gboolean gStopSearch = FALSE;

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
	GError *error = NULL;
	TrackerSparqlConnection *connection = NULL;
	TrackerSparqlCursor *cursor = NULL;
	gsize len, term, i = 1;
	gchar *query = NULL;
	gStopSearch = FALSE;

	gUpdateStatus(PluginNr, _("not found"), 0);

	gchar *startpath_uri = g_filename_to_uri(pSearchRec->StartPath, NULL, NULL);

	if (!pSearchRec->IsFindText)
		query = g_strdup_printf(NOTEXT_QUERYF, startpath_uri, pSearchRec->FileMask);
	else
		query = g_strdup_printf(TEXT_QUERYF, startpath_uri, pSearchRec->FindText, pSearchRec->FileMask);

	g_free(startpath_uri);

	connection = tracker_sparql_connection_bus_new(REMOTE_NAME, NULL, NULL, &error);

	if (connection)
	{
		cursor = tracker_sparql_connection_query(connection, query, NULL, &error);

		if (error)
		{
			gUpdateStatus(PluginNr, error->message, 0);
			g_clear_error(&error);
		}

		if (cursor)
		{
			while (!gStopSearch && tracker_sparql_cursor_next(cursor, NULL, &error))
			{
				const gchar *uri = tracker_sparql_cursor_get_string(cursor, 0, NULL);

				if (uri)
				{
					gchar *fname = g_filename_from_uri(uri, NULL, NULL);

					if (fname)
					{
						gAddFileProc(PluginNr, fname);
						gUpdateStatus(PluginNr, fname, i++);
						g_free(fname);
					}
				}
			}
		}
	}

	g_free(query);

	if (error)
	{
		gUpdateStatus(PluginNr, error->message, 0);
		g_error_free(error);

	}

	if (cursor)
		tracker_sparql_cursor_close(cursor);

	if (connection)
		tracker_sparql_connection_close(connection);

	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	gStopSearch = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{

}
