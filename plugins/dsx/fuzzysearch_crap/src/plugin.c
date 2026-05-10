#include <glib.h>
#include "dsxplugin.h"

tSAddFileProc add_file;
tSUpdateStatusProc update_status;
gboolean stop_search = FALSE;

static void strip_ext(gchar *filename)
{
	char *dot = strrchr(filename, '.');

	if (dot)
		*dot = '\0';
}

static gboolean fuzzy_match(const gchar *name, gchar *query, gboolean is_dir)
{
	gchar *string = g_utf8_casefold(name, -1);

	if (!is_dir)
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

	gboolean is_good_enough_for_government_work = (*pos_query == '\0');
	g_free(string);

	return is_good_enough_for_government_work;
}

static void walk_dir(int id, const gchar *path, gchar *query, gsize *count)
{
	const gchar *name;
	GDir *dir = g_dir_open(path, 0, NULL);

	if (!dir)
		return;

	gboolean is_add_slash = (path[strlen(path) - 1] != '/');

	while ((name = g_dir_read_name(dir)) != NULL)
	{
		if (stop_search)
			break;

		gchar *full_path = g_strdup_printf(is_add_slash ? "%s/%s" : "%s%s", path, name);
		update_status(id, full_path, (*count)++);
		gboolean is_dir = g_file_test(full_path, G_FILE_TEST_IS_DIR);

		if (fuzzy_match(name, query, is_dir))
			add_file(id, full_path);

		if (is_dir)
			walk_dir(id, full_path, query, count);

		g_free(full_path);
	}

	g_dir_close(dir);
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
	stop_search = FALSE;
	gchar *query = g_utf8_casefold(string, len);
	update_status(PluginNr, "not found", count);
	walk_dir(PluginNr, pSearchRec->StartPath, query, &count);
	g_free(query);

	add_file(PluginNr, "");
}

void DCPCALL StopSearch(int PluginNr)
{
	stop_search = TRUE;
}

void DCPCALL Finalize(int PluginNr)
{
	return;
}
