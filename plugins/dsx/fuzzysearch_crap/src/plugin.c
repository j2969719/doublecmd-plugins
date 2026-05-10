#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <glib.h>
#include "dsxplugin.h"

int plug_id;
tSAddFileProc add_file;
tSUpdateStatusProc update_status;
gsize count;
gchar *query = NULL;
gboolean is_stop_search = FALSE;


static void strip_ext(gchar *filename)
{
	char *dot = strrchr(filename, '.');

	if (dot)
		*dot = '\0';
}

int fuzzy_match(const char *path, const struct stat *buf, int flag, struct FTW *ftwbuf)
{
	if (is_stop_search)
		return 1;

	update_status(plug_id, (char*)path, count++);

	const char *name = path + ftwbuf->base;
	gchar *string = g_utf8_casefold(name, -1);

	if (flag == FTW_F)
		strip_ext(string);

	gchar *pos_string = string;
	gchar *pos_query = query;

	while (*pos_string != '\0' && *pos_query != '\0')
	{
		gunichar uchar_string = g_utf8_get_char(pos_string);
		gunichar uchar_query = g_utf8_get_char(pos_query);

		if (uchar_string == uchar_query)
			pos_query = g_utf8_next_char(pos_query);

		pos_string = g_utf8_next_char(pos_string);
	}

	if (*pos_query == '\0')
		add_file(plug_id, (char*)path);

	return 0;
}

int DCPCALL Init(tDsxDefaultParamStruct* dsp, tSAddFileProc pAddFileProc, tSUpdateStatusProc pUpdateStatus)
{
	add_file = pAddFileProc;
	update_status = pUpdateStatus;
	return 0;
}

void DCPCALL StartSearch(int PluginNr, tDsxSearchRecord* pSearchRec)
{
	char *string = pSearchRec->FileMask;
	gsize len = strlen(string);

	if (len > 0 && string[0] == '*')
	{
		string++;
		len--;
	}

	if (len > 0 && string[len - 1] == '*')
		len--;

	gsize count = 0;
	is_stop_search = FALSE;
	query = g_utf8_casefold(string, len);
	update_status(PluginNr, "not found", count);
	nftw(pSearchRec->StartPath, fuzzy_match, 69, FTW_DEPTH | FTW_PHYS);
	g_free(query);

	add_file(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	is_stop_search = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{
	return;
}
