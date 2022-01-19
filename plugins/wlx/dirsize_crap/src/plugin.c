#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <string.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"


typedef struct t_somedata
{
	GtkWidget *spin;
	GtkWidget *scroll;
	GtkWidget *list;
	gchar *path;
	GCancellable *cancel;
} somedata;

enum
{
	LIST_ICON,
	LIST_FILE,
	LIST_SIZE,
	LIST_SNUM,
	LIST_PRGV,
	N_COLUMNS
};

static gsize get_dir_size(const gchar *path, GCancellable *cancel)
{
	gsize size = 0;
	const gchar *name;
	gchar file[PATH_MAX];
	struct stat buf;

	GDir *dir = g_dir_open(path, 0, NULL);

	if (dir != NULL)
	{

		while ((name = g_dir_read_name(dir)) != NULL)
		{
			if (g_cancellable_is_cancelled(cancel))
				break;

			snprintf(file, PATH_MAX, "%s/%s", path, name);

			if (lstat(file, &buf) == 0)
			{
				if (S_ISDIR(buf.st_mode))
					size += get_dir_size(file, cancel);
				else
					size += (gsize)buf.st_size;
			}
		}

		g_dir_close(dir);
	}

	return size;
}

static GIcon* get_fileicon(const gchar *file)
{
	GFile *gfile = g_file_new_for_path(file);

	if (!gfile)
		return NULL;

	GFileInfo *fileinfo = g_file_query_info(gfile,
	                                        G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                                        0, NULL, NULL);

	if (!fileinfo)
	{
		g_object_unref(gfile);
		return NULL;
	}

	gchar *content_type = g_strdup(g_file_info_get_content_type(fileinfo));
	g_object_unref(fileinfo);
	g_object_unref(gfile);
	GIcon *icon = g_content_type_get_icon(content_type);
	g_free(content_type);
	return icon;
}

static gpointer fill_list(gpointer userdata)
{
	gsize size = 0;
	GtkTreeIter iter;
	const gchar *name;
	gchar file[PATH_MAX];
	struct stat buf;
	somedata *data = (somedata*)userdata;

	gtk_spinner_start(GTK_SPINNER(data->spin));

	gsize total = get_dir_size(data->path, data->cancel);

	if (g_cancellable_is_cancelled(data->cancel))
		return NULL;

	GtkListStore *store = gtk_list_store_new(N_COLUMNS,
	                      G_TYPE_ICON,
	                      G_TYPE_STRING,
	                      G_TYPE_STRING,
	                      G_TYPE_INT64,
	                      G_TYPE_INT);

	GDir *dir = g_dir_open(data->path, 0, NULL);

	if (dir != NULL)
	{

		while ((name = g_dir_read_name(dir)) != NULL)
		{
			gtk_list_store_append(store, &iter);
			snprintf(file, PATH_MAX, "%s/%s", data->path, name);

			if (lstat(file, &buf) == 0)
			{
				if (S_ISDIR(buf.st_mode))
					size = get_dir_size(file, data->cancel);
				else
					size = (gsize)buf.st_size;

				gint value = 0;

				if (total > 0)
					value = size * 100 / total;

				gchar *filesize = g_format_size(size);

				GIcon *icon = get_fileicon(file);
				gtk_list_store_set(store, &iter,
				                   LIST_ICON, icon,
				                   LIST_FILE, name,
				                   LIST_SIZE, filesize,
				                   LIST_SNUM, size,
				                   LIST_PRGV, value, -1);
				g_free(filesize);
				g_object_unref(icon);
			}

			if (g_cancellable_is_cancelled(data->cancel))
				break;
		}

		g_dir_close(dir);
	}

	if (!g_cancellable_is_cancelled(data->cancel))
	{
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
		                                     LIST_SNUM, GTK_SORT_DESCENDING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->list), GTK_TREE_MODEL(store));
		gtk_spinner_stop(GTK_SPINNER(data->spin));
		gtk_widget_set_visible(data->spin, FALSE);
	}

	g_object_unref(store);

	return NULL;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	if (!g_file_test(FileToLoad, G_FILE_TEST_IS_DIR))
		return NULL;

	GtkWidget *main_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), main_vbox);

	somedata *data = g_new0(somedata, 1);
	data->spin = gtk_spinner_new();
	data->scroll = gtk_scrolled_window_new(NULL, NULL);
	data->list = gtk_tree_view_new();
	data->path = g_strdup(FileToLoad);
	data->cancel = g_cancellable_new();

	GtkWidget *info_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), info_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(info_box), gtk_label_new(FileToLoad), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(info_box), data->spin, FALSE, FALSE, 1);
	gtk_container_add(GTK_CONTAINER(main_vbox), data->scroll);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(data->scroll),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(data->scroll), data->list);

	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Name"));
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "gicon", LIST_ICON, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", LIST_FILE, NULL);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(data->list), column);

	renderer = gtk_cell_renderer_progress_new();
	g_object_set(renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Size"), renderer,
	                "text", LIST_SIZE, "value", LIST_PRGV, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(data->list), column);

	gtk_widget_show_all(main_vbox);

	g_object_set_data(G_OBJECT(main_vbox), "custom-data", data);

	GThread *thread = g_thread_new("calcsize", fill_list, data);
	g_object_set_data(G_OBJECT(main_vbox), "thread", thread);

	return (HWND)main_vbox;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	somedata *data = (somedata*)g_object_get_data(G_OBJECT(ListWin), "custom-data");
	GThread *thread = (GThread*)g_object_get_data(G_OBJECT(ListWin), "thread");
	g_cancellable_cancel(data->cancel);
	g_thread_join(thread);
	g_thread_unref(thread);
	g_object_unref(data->cancel);
	gtk_widget_destroy(GTK_WIDGET(ListWin));
	g_free(data->path);
	g_free(data);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	const gchar* dir_f = "%s/langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(dir_f, &dlinfo) != 0)
	{
		setlocale(LC_ALL, "");
		gchar *plugdir = g_path_get_dirname(dlinfo.dli_fname);
		gchar *langdir = g_strdup_printf(dir_f, plugdir);
		g_free(plugdir);
		bindtextdomain(GETTEXT_PACKAGE, langdir);
		g_free(langdir);
		textdomain(GETTEXT_PACKAGE);
	}
}
