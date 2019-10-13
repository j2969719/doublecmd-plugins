#define _GNU_SOURCE
#include <glib.h>
#include <libtracker-sparql/tracker-sparql.h>
#include "dsxplugin.h"

#include <dlfcn.h>

#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

tSAddFileProc gAddFileProc;
tSUpdateStatusProc gUpdateStatus;

gboolean stop_search;

#define notext_queryf "\
SELECT nie:url(?s) WHERE \
{\
 ?s a nfo:FileDataObject .\
 ?s nie:url ?url . \
 FILTER (STRSTARTS (?url, '%s')) . \
 {\
   ?s nfo:fileName ?name .\
   FILTER (CONTAINS(LCASE(?name), LCASE('%s'))) \
 }\
}\
"
#define text_queryf "\
SELECT nie:url(?s) WHERE \
{\
 ?s a nfo:FileDataObject .\
 ?s nie:url ?url . \
 FILTER (STRSTARTS (?url, '%s')) . \
 ?s fts:match '%s' . \
 {\
   ?s nfo:fileName ?name .\
   FILTER (CONTAINS(LCASE(?name), LCASE('%s'))) \
 }\
}\
"

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

		setlocale (LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, plg_path);
		textdomain(GETTEXT_PACKAGE);
	}

	return 0;
}

void DCPCALL StartSearch(int PluginNr, tDsxSearchRecord* pSearchRec)
{
	GError *error = NULL;
	TrackerSparqlConnection *connection;
	TrackerSparqlCursor *cursor;
	gsize len, term, i = 1;
	gchar *query;
	stop_search = FALSE;

	gUpdateStatus(PluginNr, _("not found"), 0);

	if (pSearchRec->IsFindText)
		query = g_strdup_printf(text_queryf, g_filename_to_uri(pSearchRec->StartPath, NULL, NULL), pSearchRec->FindText, pSearchRec->FileMask);
	else
		query = g_strdup_printf(notext_queryf, g_filename_to_uri(pSearchRec->StartPath, NULL, NULL), pSearchRec->FileMask);

	connection = tracker_sparql_connection_get(NULL, &error);

	if (connection)
	{
		cursor = tracker_sparql_connection_query(connection, query, NULL, &error);

		if (cursor)
		{
			while (!stop_search && tracker_sparql_cursor_next(cursor, NULL, NULL))
			{
				const gchar *uri = tracker_sparql_cursor_get_string(cursor, 0, NULL);

				if (uri)
				{
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


	gAddFileProc(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	stop_search = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{

}
