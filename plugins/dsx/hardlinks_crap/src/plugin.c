#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <glib.h>
#include <sys/stat.h>
#include "dsxplugin.h"

#define EXEC_SEP "< < < < < < < < < < < < < < < < < < < < < < < < < > > > > > > > > > > > > > > > > > > > > > > > > >"

int plug_id;
tSAddFileProc add_file;
tSUpdateStatusProc update_status;
gboolean is_stop_search = FALSE;
GHashTable *hardlinks = NULL;
gsize count;
gchar *filemask = NULL; 

int find_hardlinks(const char *path, const struct stat *buf, int flag, struct FTW *ftwbuf)
{
	if (is_stop_search)
		return 1;

	update_status(plug_id, (char*)path, count++);

	if (flag == FTW_F && buf->st_nlink > 1)
	{
		if (filemask)
		{
			const char *filename = path + ftwbuf->base;

			if (!g_pattern_match_simple(filemask, filename))
				return 0;
		}

		gpointer key = GUINT_TO_POINTER((guint64)buf->st_ino);
		gpointer value = NULL;

		if (!g_hash_table_lookup_extended(hardlinks, key, NULL, &value))
		{
			value = g_ptr_array_new_with_free_func(g_free);
			g_hash_table_insert(hardlinks, key, value);
		}

		g_ptr_array_add((GPtrArray*)value, g_strdup(path));
	}

	return 0;
}

void dump_hardlinks(gpointer key, gpointer value, gpointer data)
{
	GPtrArray *paths = (GPtrArray*)value;

	for (guint i = 0; i < paths->len; i++)
		add_file(plug_id, (char*)paths->pdata[i]);

	add_file(plug_id, EXEC_SEP);

}

int DCPCALL Init(tDsxDefaultParamStruct* dsp, tSAddFileProc pAddFileProc, tSUpdateStatusProc pUpdateStatus)
{
	add_file = pAddFileProc;
	update_status = pUpdateStatus;
	return 0;
}

void DCPCALL StartSearch(int PluginNr, tDsxSearchRecord* pSearchRec)
{
	count = 0;
	plug_id = PluginNr;
	is_stop_search = FALSE;
	filemask = pSearchRec->FileMask;
	update_status(PluginNr, "not found", count);
	hardlinks = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)g_ptr_array_unref);
	nftw(pSearchRec->StartPath, find_hardlinks, 69, FTW_PHYS | FTW_MOUNT);

	if (!is_stop_search)
		g_hash_table_foreach(hardlinks, dump_hardlinks, NULL);

	g_hash_table_destroy(hardlinks);
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
