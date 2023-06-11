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
	GtkWidget *label;
	gchar *path;
	gsize total;
	guint idle;
	GCancellable *cancel;
	GtkListStore *store;
} somedata;

typedef struct t_someshit
{
	GtkWidget *label;
	guint idle;
	gchar *name;
} someshit;

enum
{
	LIST_ICON,
	LIST_FILE,
	LIST_SIZE,
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

	const gchar *content_type = g_file_info_get_content_type(fileinfo);
	GIcon *icon = g_content_type_get_icon(content_type);
	g_object_unref(fileinfo);
	g_object_unref(gfile);
	return icon;
}

static void set_progress_values(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                                GtkTreeModel *model, GtkTreeIter *iter, gpointer userdata)
{
	gsize size = 0;
	somedata *data = (somedata*)userdata;
	gtk_tree_model_get(model, iter, LIST_SIZE, &size, -1);
	gint value = 0;

	if (data->total > 0)
		value = size * 100 / data->total;

	gchar *filesize = g_format_size_full(size, G_FORMAT_SIZE_IEC_UNITS);

	g_object_set(cell, "text", filesize, NULL);
	g_object_set(cell, "value", value, NULL);
	g_free(filesize);
}

static gboolean update_progress(gpointer userdata)
{
	someshit *data = (someshit*)userdata;

	if (GTK_IS_LABEL(data->label))
		gtk_label_set_text(GTK_LABEL(data->label), data->name);

	g_free(data->name);
	data->idle = 0;

	return FALSE;
}

static gboolean update_list(gpointer userdata)
{
	somedata *data = (somedata*)userdata;
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(data->store),
	                                     LIST_SIZE, GTK_SORT_DESCENDING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->list), GTK_TREE_MODEL(data->store));
	gtk_spinner_stop(GTK_SPINNER(data->spin));
	gtk_widget_set_visible(data->spin, FALSE);
	gchar *filesize = g_format_size_full(data->total, G_FORMAT_SIZE_IEC_UNITS);
	gtk_label_set_text(GTK_LABEL(data->label), filesize);
	g_free(filesize);
	data->idle = 0;
	return FALSE;
}

static gpointer fill_list(gpointer userdata)
{
	gsize size = 0;
	GtkTreeIter iter;
	const gchar *name;
	gchar file[PATH_MAX];
	struct stat buf;
	someshit *udata = NULL;
	somedata *data = (somedata*)userdata;

	if (g_cancellable_is_cancelled(data->cancel))
		return NULL;

	GDir *dir = g_dir_open(data->path, 0, NULL);

	if (dir != NULL)
	{

		while ((name = g_dir_read_name(dir)) != NULL)
		{
			someshit *udata = g_new0(someshit, 1);
			udata->name = g_strdup(name);
			udata->label = data->label;
			udata->idle = gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE, update_progress, udata, g_free);

			snprintf(file, PATH_MAX, "%s/%s", data->path, name);

			if (lstat(file, &buf) == 0)
			{
				if (S_ISDIR(buf.st_mode))
					size = get_dir_size(file, data->cancel);
				else
					size = (gsize)buf.st_size;

				data->total += size;

				GIcon *icon = get_fileicon(file);
				gtk_list_store_append(data->store, &iter);
				gtk_list_store_set(data->store, &iter,
				                   LIST_ICON, icon,
				                   LIST_FILE, name,
				                   LIST_SIZE, size, -1);
				g_object_unref(icon);
			}

			if (g_cancellable_is_cancelled(data->cancel))
				break;
		}

		g_dir_close(dir);
	}


	if (!g_cancellable_is_cancelled(data->cancel))
	{
		if (udata && udata->idle)
			g_source_remove(udata->idle);

		data->idle = gdk_threads_add_idle(update_list, data);
	}

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
	data->label = gtk_label_new(NULL);
	data->scroll = gtk_scrolled_window_new(NULL, NULL);
	data->list = gtk_tree_view_new();
	data->path = g_strdup(FileToLoad);
	data->cancel = g_cancellable_new();

	data->store = gtk_list_store_new(N_COLUMNS,
	                                 G_TYPE_ICON,
	                                 G_TYPE_STRING,
	                                 G_TYPE_INT64);

	GtkWidget *path_label = gtk_label_new(FileToLoad);
	gtk_misc_set_alignment(GTK_MISC(path_label), 0, 0.5);
	GtkWidget *info_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), info_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(info_box), path_label, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(info_box), data->spin, FALSE, FALSE, 1);
	gtk_box_pack_end(GTK_BOX(info_box), data->label, FALSE, FALSE, 5);
	gtk_container_add(GTK_CONTAINER(main_vbox), data->scroll);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(data->scroll),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(data->scroll), data->list);

	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_max_width(column, 256);
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
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Size"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, set_progress_values, data, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(data->list), column);

	gtk_widget_show_all(main_vbox);

	g_object_set_data(G_OBJECT(main_vbox), "custom-data", data);

	gtk_spinner_start(GTK_SPINNER(data->spin));
	GThread *thread = g_thread_new("calcsize", fill_list, data);
	g_object_set_data(G_OBJECT(main_vbox), "thread", thread);

	return (HWND)main_vbox;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	somedata *data = (somedata*)g_object_get_data(G_OBJECT(ListWin), "custom-data");
	GThread *thread = (GThread*)g_object_get_data(G_OBJECT(ListWin), "thread");

	if (data->idle)
		g_source_remove(data->idle);

	g_cancellable_cancel(data->cancel);
	g_thread_join(thread);
	g_thread_unref(thread);
	g_object_unref(data->cancel);
	g_object_unref(data->store);
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
