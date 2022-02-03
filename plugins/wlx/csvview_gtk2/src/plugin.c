#include <gtk/gtk.h>
#include <enca.h>
#include <locale.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

static char g_lang[3];
gboolean g_enca = TRUE;
gboolean g_quoted = TRUE;
gboolean g_grid = FALSE;

static gboolean quotes_not_closed(gchar *string)
{
	gint count = 0;

	while (*string)
		if (*string++ == '"')
			count++;

	if (count % 2 == 0)
		return FALSE;

	return TRUE;
}

static gchar *unqoute(gchar *string)
{
	gchar *target = NULL;

	if (string[0] == '"' && string[strlen(string) - 1] == '"')
		target = g_strndup(string + 1, strlen(string) - 2);
	else
		target = g_strdup(string);

	gchar **split = g_strsplit(target, "\"\"", -1);
	gchar *result = g_strjoinv("\"", split);

	g_free(target);
	g_strfreev(split);

	return result;
}

static gchar *get_value(gchar ***p, gchar *separator)
{
	gchar *result = NULL;
	gchar **itm = *p;

	if (g_quoted && separator[0] != '\t' && *itm[0] == '"' && quotes_not_closed(*itm))
	{
		gchar **nitm = itm + 1;
		gchar *cat = g_strdup(*itm);

		while (*nitm != NULL)
		{
			gchar *tmp = g_strdup(cat);
			cat = g_strjoin(separator, tmp, *nitm, NULL);
			g_free(tmp);

			if (cat && cat[strlen(cat) - 1] == '"' && !quotes_not_closed(cat))
			{
				result = unqoute(cat);
				*p = nitm;
				break;
			}

			nitm++;
		}

		g_free(cat);

		if (*p != nitm)
			result = g_strdup(*itm);

	}
	else if (g_quoted && separator[0] != '\t')
	{
		result = unqoute(*itm);
	}
	else
		result = strdup(*itm);

	return result;
}

static GtkListStore *parse_file(char* FileToLoad, GtkTreeView *list)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store = NULL;
	GtkTreeIter iter;
	gsize columns = 0;
	GType *types;
	gchar *separator, *buffer, enc[256];
	gchar *seps[] = { ",", ";", "\t", NULL };
	gchar **lines, **row;
	gsize bytes_read, len, i;
	EncaAnalyser analyser;
	EncaEncoding encoding;

	gtk_tree_view_set_model(list, NULL);

	if (!g_file_get_contents(FileToLoad, &buffer, &bytes_read, NULL))
		return NULL;

	if (bytes_read == 0)
	{
		g_free(buffer);
		return NULL;
	}

	if (g_enca)
	{
		analyser = enca_analyser_alloc(g_lang);

		if (analyser)
		{
			enca_set_threshold(analyser, 1.38);
			enca_set_multibyte(analyser, 1);
			enca_set_ambiguity(analyser, 1);
			enca_set_garbage_test(analyser, 1);
			enca_set_filtering(analyser, 0);

			encoding = enca_analyse(analyser, (unsigned char*)buffer, (size_t)bytes_read);
			g_strlcpy(enc, enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV), 256);
			enca_analyser_free(analyser);
		}

		gchar *converted = g_convert_with_fallback(buffer, bytes_read, "UTF-8", enc, NULL, NULL, &len, NULL);

		if (converted != NULL)
		{
			g_free(buffer);
			buffer = converted;
		}
	}

	if (!buffer)
		return NULL;

	lines = g_regex_split_simple("[\r\n]+", buffer, 0, 0);
	g_free(buffer);

	if (!lines)
		return NULL;

	for (gchar **chr = seps; *chr != NULL; chr++)
	{
		row = g_strsplit(*lines, *chr, -1);

		if (row)
			columns = g_strv_length(row);
		else
			continue;

		if (columns > 1)
		{
			GList *cols = gtk_tree_view_get_columns(list);

			for (GList *l = cols; l != NULL; l = l->next)
			{
				if (GTK_IS_TREE_VIEW_COLUMN(l->data))
					gtk_tree_view_remove_column(list, GTK_TREE_VIEW_COLUMN(l->data));
			}

			separator = *chr;
			types = (GType*)g_malloc((columns + 1) * sizeof(GType));

			for (i = 0; i <= columns; i++)
				types[i] = G_TYPE_STRING;

			store = gtk_list_store_newv(columns + 1, types);
			g_free(types);

			i = 0;

			for (gchar **itm = row; *itm != NULL; itm++)
			{
				renderer = gtk_cell_renderer_text_new();
				g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);

				gchar *header = get_value(&itm, separator);
				column = gtk_tree_view_column_new_with_attributes(header, renderer, "text", i, NULL);
				g_free(header);

				gtk_tree_view_column_set_sort_column_id(column, i);
				gtk_tree_view_append_column(list, column);
				i++;
			}

			renderer = gtk_cell_renderer_text_new();
			column = gtk_tree_view_column_new_with_attributes("", renderer, "text", i, NULL);
			gtk_tree_view_column_set_sort_column_id(column, i);
			gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

			break;
		}

		g_strfreev(row);
	}

	if (columns < 2)
	{
		if (G_IS_OBJECT(store))
			g_object_unref(store);

		g_strfreev(lines);
		return NULL;
	}

	for (gchar **line = lines + 1; *line != NULL; line++)
	{
		i = 0;

		if (strlen(*line) < 1)
			continue;

		row = g_strsplit(*line, separator, -1);

		if (row)
		{
			gtk_list_store_append(store, &iter);
			GString *trash = g_string_new(NULL);

			for (gchar **itm = row; *itm != NULL; itm++)
			{
				if (i >= columns)
				{
					trash = g_string_append(trash, ", ");
					trash = g_string_append(trash, *itm);
				}
				else
				{
					gchar *value = get_value(&itm, separator);
					gtk_list_store_set(store, &iter, i++, value, -1);
					g_free(value);
				}
			}

			gtk_list_store_set(store, &iter, i++, g_string_free(trash, FALSE), -1);
			g_strfreev(row);
		}
	}

	g_strfreev(lines);

	return store;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *list;

	list = gtk_tree_view_new();

	GtkListStore *store = parse_file(FileToLoad, GTK_TREE_VIEW(list));

	if (!store)
	{
		gtk_widget_destroy(list);
		return NULL;
	}

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(gFix), scroll);

	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));
	g_object_unref(store);


	if (g_grid)
		gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(list), GTK_TREE_VIEW_GRID_LINES_BOTH);
	else
		gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(list), GTK_TREE_VIEW_GRID_LINES_NONE);


	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), list);
	g_object_set_data(G_OBJECT(gFix), "list", list);

	gtk_widget_show_all(gFix);
	gtk_widget_grab_focus(scroll);

	return gFix;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *list = (GtkWidget*)g_object_get_data(G_OBJECT(PluginWin), "list");

	GtkListStore *store = parse_file(FileToLoad, GTK_TREE_VIEW(list));

	if (!store)
		return LISTPLUGIN_ERROR;

	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));
	g_object_unref(store);
	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, "(EXT=\"CSV\" | EXT=\"TSV\") & SIZE<30000000", maxlen - 1);
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

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	GKeyFile *cfg;
	char cfg_path[PATH_MAX];

	g_strlcpy(cfg_path, dps->DefaultIniName, PATH_MAX);

	char *pos = strrchr(cfg_path, '/');

	if (pos)
		strcpy(pos + 1, "j2969719.ini");

	cfg = g_key_file_new();

	g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL);

	if (g_key_file_has_group(cfg, "csvviewer"))
	{
		g_key_file_remove_group(cfg, "csvviewer", NULL);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
	}

	if (!g_key_file_has_key(cfg, "csvview", "draw_grid", NULL))
	{
		g_key_file_set_boolean(cfg, "csvview", "draw_grid", g_grid);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
	}
	else
		g_grid = g_key_file_get_boolean(cfg, "csvview", "draw_grid", NULL);

	if (!g_key_file_has_key(cfg, "csvview", "enca", NULL))
	{
		g_key_file_set_boolean(cfg, "csvview", "enca", g_enca);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
	}
	else
		g_enca = g_key_file_get_boolean(cfg, "csvview", "enca", NULL);

	if (!g_key_file_has_key(cfg, "csvview", "doublequoted", NULL))
	{
		g_key_file_set_boolean(cfg, "csvview", "doublequoted", g_quoted);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
	}
	else
		g_quoted = g_key_file_get_boolean(cfg, "csvview", "doublequoted", NULL);

	if (!g_key_file_has_key(cfg, "csvview", "enca_lang", NULL))
	{
		snprintf(g_lang, 3, "%s", setlocale(LC_ALL, ""));
		g_key_file_set_string(cfg, "csvview", "enca_lang", g_lang);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
	}
	else
	{
		gchar *val = g_key_file_get_string(cfg, "csvview", "enca_lang", NULL);

		if (val)
		{
			g_strlcpy(g_lang, val, 3);
			g_free(val);
		}
		else
			g_strlcpy(g_lang, "__", 3);
	}

	g_key_file_free(cfg);
}
