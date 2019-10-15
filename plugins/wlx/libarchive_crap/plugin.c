#include <gtk/gtk.h>
#include <archive.h>
#include <archive_entry.h>
#include "wlxplugin.h"

gchar *get_owner_str(struct archive_entry *entry)
{
	const gchar *group = archive_entry_gname_utf8(entry);
	const gchar *user = archive_entry_uname_utf8(entry);

	if (!group || !user)
		return NULL;
	else
		return g_strdup_printf("%s/%s", user, group);
}

gchar *get_datetime_str(struct archive_entry *entry)
{
	time_t	e_mtime = archive_entry_mtime(entry);
	return g_date_time_format(g_date_time_new_from_unix_local(e_mtime), "%d.%m.%Y %k:%M");
}

GdkPixbuf *get_emblem_pixbuf(char *name, int size)
{
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(theme, name, size, 0);
	GdkPixbuf *pixbuf = gtk_icon_info_load_icon(icon_info, NULL);
	gtk_icon_info_free(icon_info);
	return pixbuf;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *versions;
	GtkWidget *infolabel;
	GtkWidget *list;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	GtkTreeIter iter;

	struct archive *a;
	struct archive_entry *entry;
	int r;
	gchar *info = NULL;
	gboolean symlinks = FALSE;
	gboolean hardlinks = FALSE;
	gboolean ownercolumn = FALSE;
	size_t totalsize = 0;



	a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	r = archive_read_open_filename(a, FileToLoad, 10240);

	if (r != ARCHIVE_OK)
		return NULL;

	enum
	{
		LIST_ICON,
		LIST_FILE,
		LIST_SIZE,
		LIST_DATE,
		LIST_ATTR,
		LIST_OWNR,
		LIST_SLNK,
		LIST_HLNK,
		N_COLUMNS
	};

	store = gtk_list_store_new(N_COLUMNS,
	                           GDK_TYPE_PIXBUF,
	                           G_TYPE_STRING,
	                           G_TYPE_INT64,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING);

	while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK || r == ARCHIVE_WARN)
	{
		gtk_list_store_append(store, &iter);

		const gchar *pathname = archive_entry_pathname(entry);
		GdkPixbuf *emblem = NULL;
		const gchar *symlink = archive_entry_symlink_utf8(entry);

		if (symlink && !symlinks)
			symlinks = TRUE;

		const gchar *hardlink = archive_entry_hardlink_utf8(entry);

		if (hardlink && !hardlinks)
			hardlinks = TRUE;

		totalsize = totalsize + archive_entry_size(entry);

		//if (pathname[strlen(pathname) - 1] == '/')
		if (archive_entry_filetype(entry) == AE_IFDIR)
			emblem = get_emblem_pixbuf("folder", 16);
		else if (symlink || hardlink)
			emblem = get_emblem_pixbuf("emblem-symbolic-link", 16);
		else if (archive_entry_is_data_encrypted(entry) == 1)
			emblem = get_emblem_pixbuf("emblem-readonly", 16);

		gchar *owner = get_owner_str(entry);

		if (owner && !ownercolumn)
			ownercolumn = TRUE;

		gtk_list_store_set(store, &iter,
		                   LIST_ICON, emblem,
		                   LIST_FILE, pathname,
		                   LIST_SIZE, archive_entry_size(entry),
		                   LIST_DATE, get_datetime_str(entry),
		                   LIST_ATTR, archive_entry_strmode(entry),
		                   LIST_OWNR, owner,
		                   LIST_SLNK, symlink,
		                   LIST_HLNK, hardlink,
		                   -1);

	}


	if (archive_format(a) == ARCHIVE_FORMAT_EMPTY)
	{
		archive_read_close(a);
		archive_read_free(a);
		return NULL;
	}

	if (r != ARCHIVE_EOF)
	{
		info = g_strdup(archive_error_string(a));
		info = g_strdup_printf("ERROR: %s", info ? info : "unknown error");
	}

	if (!info)
	{
		int fc = archive_filter_count(a);
		gchar *filters = NULL;

		for (int i = 0; i < fc; i++)
		{
			const char *fn = archive_filter_name(a, i);

			if (strcmp(fn, "none") != 0)
				if (filters)
					filters = g_strdup_printf("%s, %s", filters, fn);
				else
					filters = g_strdup(fn);
		}

		if (filters)
			info = g_strdup_printf("%s (filter(s): %s), %d file(s)", archive_format_name(a), filters, archive_file_count(a));
		else
			info = g_strdup_printf("%s, %d file(s)",  archive_format_name(a), archive_file_count(a));


		if (filters)
			g_free(filters);

	}

	archive_read_close(a);
	archive_read_free(a);

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);

	infolabel = gtk_label_new(info);
	gtk_box_pack_start(GTK_BOX(gFix), infolabel, FALSE, FALSE, 1);

	struct stat buf;

	if (totalsize > 0 && stat(FileToLoad, &buf) == 0)
	{
		GtkWidget *compr = gtk_progress_bar_new();

		gdouble res = (gdouble)buf.st_size / (gdouble)totalsize;

		if (res > 1)
			res = 0;
		else
			res = 1 - res;

		//gtk_progress_bar_set_text(GTK_PROGRESS_BAR(compr), g_strdup_printf("%d / %d", buf.st_size, totalsize));
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(compr), res);
		gtk_box_pack_start(GTK_BOX(gFix), compr, FALSE, FALSE, 1);
	}

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	list = gtk_tree_view_new();

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "file");
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", LIST_ICON, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", LIST_FILE, NULL);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, LIST_FILE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("size", renderer, "text", LIST_SIZE, NULL);
	gtk_tree_view_column_set_sort_column_id(column, LIST_SIZE);
	gtk_tree_view_column_set_sort_order(column, GTK_SORT_DESCENDING);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("date", renderer, "text", LIST_DATE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("attr", renderer, "text", LIST_ATTR, NULL);
	gtk_tree_view_column_set_sort_column_id(column, LIST_ATTR);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	if (ownercolumn)
	{
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("owner", renderer, "text", LIST_OWNR, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	}

	if (symlinks)
	{
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("symlink", renderer, "text", LIST_SLNK, NULL);
		gtk_tree_view_column_set_sort_column_id(column, LIST_SLNK);
		gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	}

	if (hardlinks)
	{
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("hardlink", renderer, "text", LIST_HLNK, NULL);
		gtk_tree_view_column_set_sort_column_id(column, LIST_HLNK);
		gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));
	gtk_container_add(GTK_CONTAINER(scroll), list);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	versions = gtk_label_new(archive_version_details());
	gtk_box_pack_start(GTK_BOX(gFix), versions, FALSE, FALSE, 1);
	gtk_misc_set_alignment(GTK_MISC(versions), 1, 0.5);
	gtk_widget_show_all(gFix);
	return gFix;

}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}
