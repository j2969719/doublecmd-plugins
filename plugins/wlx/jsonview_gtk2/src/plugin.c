#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <json-glib/json-glib.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

typedef struct sCustomData
{
	GtkTreeStore *store;
	GtkTreeIter child;
	GtkTreeIter parent;
	GtkWidget *tree;
} CustomData;

enum
{
	KEY = 0,
	VALUE,
	TYPE,
	N_COLS
};

gboolean g_expand = TRUE;
gboolean g_sorting = FALSE;
gboolean g_filename = TRUE;
static void check_value(JsonNode *node, CustomData *data, GtkTreeIter parent);

static void walk_array(JsonArray *array, guint index_, JsonNode *element_node, gpointer user_data)
{
	CustomData *data = (CustomData*)user_data;
	gchar *element = g_strdup_printf("[%d] ", index_);
	GtkTreeIter parent = data->parent;
	gtk_tree_store_append(data->store, &data->child, &data->parent);
	gtk_tree_store_set(data->store, &data->child, KEY, element, -1);
	g_free(element);
	check_value(element_node, data, data->child);
	data->parent = parent;
}

static void walk_object(JsonObject *object, const gchar *member_name, JsonNode *member_node, gpointer user_data)
{
	CustomData *data = (CustomData*)user_data;
	GtkTreeIter parent = data->parent;
	gtk_tree_store_append(data->store, &data->child, &data->parent);
	gtk_tree_store_set(data->store, &data->child, KEY, member_name, -1);
	check_value(member_node, data, data->child);
	data->parent = parent;
}

static void check_value(JsonNode *node, CustomData *data, GtkTreeIter parent)
{
	JsonNodeType type = json_node_get_node_type(node);

	switch (type)
	{
	case JSON_NODE_OBJECT:
		data->parent = parent;
		gtk_tree_store_set(data->store, &parent, TYPE, _("Object"), -1);
		json_object_foreach_member(json_node_get_object(node), walk_object, (gpointer)data);
		break;

	case JSON_NODE_ARRAY:
		data->parent = parent;
		gtk_tree_store_set(data->store, &parent, TYPE, _("Array"), -1);
		json_array_foreach_element(json_node_get_array(node), walk_array, (gpointer)data);
		break;

	case JSON_NODE_VALUE:
		if (json_node_get_value_type(node) == G_TYPE_STRING)
		{
			gtk_tree_store_set(data->store, &parent, VALUE, json_node_get_string(node), -1);
			gtk_tree_store_set(data->store, &parent, TYPE, _("String"), -1);
		}
		else if (json_node_get_value_type(node) == G_TYPE_INT64)
		{
			gchar *str = g_strdup_printf("%ld", (long)json_node_get_int(node));
			gtk_tree_store_set(data->store, &parent, VALUE, str, -1);
			g_free(str);
			gtk_tree_store_set(data->store, &parent, TYPE, _("Integer"), -1);
		}
		else if (json_node_get_value_type(node) == G_TYPE_DOUBLE)
		{
			gchar *str = g_strdup_printf("%f", json_node_get_double(node));
			gtk_tree_store_set(data->store, &parent, VALUE, str, -1);
			g_free(str);
			gtk_tree_store_set(data->store, &parent, TYPE, _("Double"), -1);
		}
		else if (json_node_get_value_type(node) == G_TYPE_BOOLEAN)
		{
			gchar *str = g_strdup_printf("%s", json_node_get_boolean(node) ? _("True") : _("False"));
			gtk_tree_store_set(data->store, &parent, VALUE, str, -1);
			g_free(str);
			gtk_tree_store_set(data->store, &parent, TYPE, _("Boolean"), -1);
		}
		else
			gtk_tree_store_set(data->store, &parent, TYPE, _("Undefined"), -1);

		break;

	case JSON_NODE_NULL:
		gtk_tree_store_set(data->store, &parent, TYPE, _("Null"), -1);
		break;
	}
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	JsonParser *parser;
	GError *err = NULL;
	CustomData *data;
	struct stat st;

	if (lstat(FileToLoad, &st) == -1 || st.st_size == 0)
		return NULL;

	parser = json_parser_new();
	json_parser_load_from_file(parser, FileToLoad, &err);

	if (err)
	{
		g_error_free(err);
		g_object_unref(parser);
		return NULL;
	}

	JsonNode *root = json_parser_get_root(parser);
	JsonNodeType root_type = json_node_get_node_type(root);

	if (root_type != JSON_NODE_OBJECT && root_type != JSON_NODE_ARRAY)
	{
		g_object_unref(parser);
		return NULL;
	}

	data = g_new0(CustomData, 1);
	data->store = gtk_tree_store_new(N_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_store_append(data->store, &data->child, NULL);

	if (g_filename)
	{
		gchar *fname = g_path_get_basename(FileToLoad);
		gtk_tree_store_set(data->store, &data->child, KEY, fname, -1);
		g_free(fname);
	}
	else
		gtk_tree_store_set(data->store, &data->child, KEY, _("Root"), -1);

	check_value(root, data, data->child);

	g_object_unref(parser);

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	g_object_set_data_full(G_OBJECT(gFix), "custom-data", data, (GDestroyNotify)g_free);

	data->tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(data->store));

	GValue val = G_VALUE_INIT;
	g_value_init(&val, G_TYPE_BOOLEAN);
	g_value_set_boolean(&val, TRUE);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Node"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(data->tree), column);

	if (g_sorting)
		gtk_tree_view_column_set_sort_column_id(column, KEY);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", KEY);
	g_object_set_property(G_OBJECT(renderer), "editable", &val);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Value"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(data->tree), column);

	if (g_sorting)
		gtk_tree_view_column_set_sort_column_id(column, VALUE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", VALUE);
	g_object_set_property(G_OBJECT(renderer), "editable", &val);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Type"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(data->tree), column);

	if (g_sorting)
		gtk_tree_view_column_set_sort_column_id(column, TYPE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", TYPE);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), data->tree);

	if (g_expand)
		gtk_tree_view_expand_all(GTK_TREE_VIEW(data->tree));

	g_value_unset(&val);
	gtk_widget_show_all(gFix);
	gtk_widget_grab_focus(scroll);

	return gFix;

}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	JsonParser *parser;
	GError *err = NULL;
	struct stat st;

	if (lstat(FileToLoad, &st) == -1 || st.st_size == 0)
		return LISTPLUGIN_ERROR;

	parser = json_parser_new();
	json_parser_load_from_file(parser, FileToLoad, &err);

	if (err)
	{
		g_error_free(err);
		g_object_unref(parser);
		return LISTPLUGIN_ERROR;
	}

	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(PluginWin), "custom-data");
	gtk_tree_store_clear(data->store);

	gtk_tree_store_append(data->store, &data->child, NULL);

	if (g_filename)
	{
		gchar *fname = g_path_get_basename(FileToLoad);
		gtk_tree_store_set(data->store, &data->child, KEY, fname, -1);
		g_free(fname);
	}
	else
		gtk_tree_store_set(data->store, &data->child, KEY, _("Root"), -1);

	check_value(json_parser_get_root(parser), data, data->child);

	if (g_expand)
		gtk_tree_view_expand_all(GTK_TREE_VIEW(data->tree));

	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(data->tree));

	g_object_unref(parser);
	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(ListWin), "custom-data");
	gtk_tree_store_clear(data->store);
	g_object_unref(data->store);
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	if (Command == lc_copy)
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
		                       gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY)), -1);

	return LISTPLUGIN_OK;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, "SIZE<30000000", maxlen - 1);
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	GKeyFile *cfg;
	char cfg_path[PATH_MAX];
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

	g_strlcpy(cfg_path, dps->DefaultIniName, PATH_MAX);

	char *pos = strrchr(cfg_path, '/');

	if (pos)
		strcpy(pos + 1, "j2969719.ini");

	cfg = g_key_file_new();

	g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL);

	if (!g_key_file_has_key(cfg, "jsonview", "tree_expand", NULL))
	{
		g_key_file_set_boolean(cfg, "jsonview", "tree_expand", g_expand);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
	}
	else
		g_expand = g_key_file_get_boolean(cfg, "jsonview", "tree_expand", NULL);

	if (!g_key_file_has_key(cfg, "jsonview", "sorting", NULL))
	{
		g_key_file_set_boolean(cfg, "jsonview", "sorting", g_sorting);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
	}
	else
		g_sorting = g_key_file_get_boolean(cfg, "jsonview", "sorting", NULL);

	if (!g_key_file_has_key(cfg, "jsonview", "show_filename", NULL))
	{
		g_key_file_set_boolean(cfg, "jsonview", "show_filename", g_filename);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
	}
	else
		g_filename = g_key_file_get_boolean(cfg, "jsonview", "show_filename", NULL);

	g_key_file_free(cfg);
}
