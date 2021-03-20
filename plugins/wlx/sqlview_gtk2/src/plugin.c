#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <sqlite3.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

typedef struct sCustomData
{
	GtkListStore *store;
	GtkWidget *list;
	GtkWidget *tables;
	GtkWidget *label;
	sqlite3 *db;
	gboolean exec;
} CustomData;

static const gchar g_table_query[] = "SELECT name FROM sqlite_master WHERE type='table'";


static int query_exec_cb(void *p, int count, char **row, char **columns)
{
	GtkTreeIter iter;
	CustomData *data = (CustomData*)p;

	gtk_list_store_append(data->store, &iter);

	for (int i = 0; i < count; i++)
		gtk_list_store_set(data->store, &iter, i, row[i] ? row[i] : "", -1);

	return 0;
}

static int tables_cb(void *tables, int count, char **values, char **names)
{
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tables), values[0]);
	return 0;
}

static void update_view(CustomData *data)
{
	char *err = NULL;
	const gchar *query = gtk_label_get_text(GTK_LABEL(data->label));
	sqlite3_stmt *stmt = NULL;

	if (!query)
		return;

	if (GTK_IS_LIST_STORE(data->store))
		gtk_list_store_clear(data->store);

	GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(data->list));

	for (GList *l = columns; l != NULL; l = l->next)
	{
		if (GTK_IS_TREE_VIEW_COLUMN(l->data))
			gtk_tree_view_remove_column(GTK_TREE_VIEW(data->list), GTK_TREE_VIEW_COLUMN(l->data));
	}

	if (sqlite3_prepare_v2(data->db, query, -1, &stmt, NULL) == SQLITE_OK)
	{
		int count = sqlite3_column_count(stmt);

		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;

		GType *types = (GType*)g_malloc((count) * sizeof(GType));

		for (int i = 0; i < count; i++)
			types[i] = G_TYPE_STRING;

		if (G_IS_OBJECT(data->store))
			g_object_unref(data->store);

		data->store = gtk_list_store_newv(count, types);

		GValue val = G_VALUE_INIT;
		g_value_init(&val, G_TYPE_BOOLEAN);
		g_value_set_boolean(&val, TRUE);

		for (int i = 0; i < count; i++)
		{
			gchar *name = g_strdup_printf("%s", sqlite3_column_name(stmt, i));
			column = gtk_tree_view_column_new();
			gtk_tree_view_column_set_title(column, name);
			gtk_tree_view_append_column(GTK_TREE_VIEW(data->list), column);
			renderer = gtk_cell_renderer_text_new();
			gtk_tree_view_column_pack_start(column, renderer, TRUE);
			gtk_tree_view_column_add_attribute(column, renderer, "text", i);
			gtk_tree_view_column_set_sort_column_id(column, i);
			g_object_set_property(G_OBJECT(renderer), "editable", &val);
			g_object_set_property(G_OBJECT(renderer), "single-paragraph-mode", &val);
			g_free(name);
		}

		g_value_unset(&val);
		g_free(types);

	}

	sqlite3_finalize(stmt);

	data->exec  = TRUE;

	if (sqlite3_exec(data->db, query, query_exec_cb, data, &err) != SQLITE_OK)
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(data->list))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, err ? err : "unknown error");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		sqlite3_free(err);
		data->exec = FALSE;
		return;
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(data->list), GTK_TREE_MODEL(data->store));

	data->exec = FALSE;
}

static void query_clicked_cb(GtkButton *button, CustomData *data)
{
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Query"), GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(data->list))),
	                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
	                    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 500, -1);
	GtkWidget *box = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(box), 5);
	GtkWidget *label = gtk_label_new(_("Please enter a new query and press OK to execute it"));
	GtkWidget *entry = gtk_entry_new();
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_entry_set_text(GTK_ENTRY(entry), gtk_label_get_text(GTK_LABEL(data->label)));
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), box, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT && !data->exec)
	{
		gtk_label_set_text(GTK_LABEL(data->label), gtk_entry_get_text(GTK_ENTRY(entry)));
		update_view(data);
	}

	gtk_widget_destroy(dialog);
}

static void tables_changed_cb(GtkComboBoxText *combo_box, CustomData *data)
{
	gchar *text = gtk_combo_box_text_get_active_text(combo_box);

	if (text && !data->exec)
	{
		gchar *query = g_strdup_printf("SELECT * FROM %s;", text);
		gtk_label_set_text(GTK_LABEL(data->label), query);
		g_free(text);
		g_free(query);
		update_view(data);
	}
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *controls;
	GtkWidget *button;
	CustomData *data;
	char *err = NULL;

	GFile *gfile = g_file_new_for_path(FileToLoad);

	if (!gfile)
		return NULL;

	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);

	if (!fileinfo)
	{
		g_object_unref(gfile);
		return NULL;
	}

	if (g_strcmp0(g_file_info_get_content_type(fileinfo), "application/vnd.sqlite3") != 0)
	{
		g_object_unref(fileinfo);
		g_object_unref(gfile);
		return NULL;
	}

	g_object_unref(fileinfo);
	g_object_unref(gfile);

	data = g_new0(CustomData, 1);

	if (sqlite3_open_v2(FileToLoad, &data->db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK)
	{
		g_print("sqlview_gtk2.wlx: %s\n", sqlite3_errmsg(data->db));
		sqlite3_close(data->db);
		g_free(data);
		return NULL;
	}

	data->tables = gtk_combo_box_text_new();

	if (sqlite3_exec(data->db, g_table_query, tables_cb, data->tables, &err) != SQLITE_OK)
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, err ? err : "unknown error");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		gtk_widget_destroy(data->tables);
		g_free(data);
		sqlite3_free(err);
		return NULL;
	}

	data->label = gtk_label_new(NULL);
	button = gtk_button_new_with_label(_("Query"));

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	controls = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(controls), gtk_label_new(_("Table:")), FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(controls), data->tables, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(controls), data->label, TRUE, TRUE, 10);
	gtk_box_pack_end(GTK_BOX(controls), button, FALSE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(gFix), controls, FALSE, FALSE, 5);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	data->list = gtk_tree_view_new();
	gtk_widget_show(data->list);
	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(data->list), GTK_TREE_VIEW_GRID_LINES_BOTH);
	gtk_container_add(GTK_CONTAINER(scroll), data->list);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(query_clicked_cb), data);
	g_signal_connect(G_OBJECT(data->tables), "changed", G_CALLBACK(tables_changed_cb), data);

	gtk_combo_box_set_active(GTK_COMBO_BOX(data->tables), 0);
	g_object_set_data_full(G_OBJECT(gFix), "custom-data", data, g_free);

	gtk_widget_show_all(gFix);
	gtk_widget_grab_focus(scroll);

	return gFix;

}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(ListWin), "custom-data");

	if (GTK_IS_LIST_STORE(data->store))
	{
		gtk_list_store_clear(data->store);
		g_object_unref(data->store);
	}

	sqlite3_close(data->db);
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(ListWin), "custom-data");

	query_clicked_cb(NULL, data);

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	if (Command == lc_copy)
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
		                       gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY)), -1);

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
