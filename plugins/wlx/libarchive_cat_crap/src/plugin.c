#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <archive.h>
#include <archive_entry.h>
#include <string.h>

#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#define GETTEXT_PACKAGE "plugins"

#include "wlxplugin.h"

static gchar gFont[MAX_PATH] = "mono 12";

static void show_error(GtkWidget *plugwin, GtkMessageType  type, char *frmt, char *string)
{
	GtkWidget *dialog = gtk_message_dialog_new(plugwin ? GTK_WINDOW(gtk_widget_get_toplevel(plugwin)) : NULL,
	                    GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, frmt, string);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static gboolean probe_file(char *filename)
{
	gboolean result = TRUE;
	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_raw(a);

	int r = archive_read_open_filename(a, filename, 10240);

	if (r != ARCHIVE_OK)
		result = FALSE;

	archive_read_close(a);
	archive_read_free(a);

	return result;
}

static gboolean open_file(gpointer userdata)
{
	size_t size;
	const void *buff;
	la_int64_t offset;
	GtkTextIter iter;

	GtkTextBuffer *text_buf = (GtkTextBuffer*)userdata;
	gchar *filename = g_object_get_data(G_OBJECT(text_buf), "filename");
	GtkWidget *view = g_object_get_data(G_OBJECT(text_buf), "text_view");

	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_raw(a);
	struct archive_entry *entry;

	int r = archive_read_open_filename(a, filename, 10240);

	if (r != ARCHIVE_OK)
	{
		archive_read_close(a);
		archive_read_free(a);
		show_error(view, GTK_MESSAGE_ERROR, _("libarchive: failed to read %s"), filename);
		return FALSE;
	}

	if ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK || r == ARCHIVE_WARN)
	{
		while ((r = archive_read_data_block(a, &buff, &size, &offset)) != ARCHIVE_EOF)
		{
			gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(text_buf), &iter);
			gtk_text_buffer_insert(GTK_TEXT_BUFFER(text_buf), &iter, (const gchar*)buff, (gint)size);

			if (r < ARCHIVE_OK)
			{
				show_error(view, GTK_MESSAGE_ERROR, "libarchive: %s", (char*)archive_error_string(a));
				break;
			}
		}
	}

	if (r != ARCHIVE_EOF)
		show_error(view, GTK_MESSAGE_ERROR, "libarchive: %s", (char*)archive_error_string(a));

	archive_read_close(a);
	archive_read_free(a);

	return FALSE;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	if (!probe_file(FileToLoad))
		return NULL;

	GtkTextBuffer *text_buf = gtk_text_buffer_new(NULL);
	g_object_set_data_full(G_OBJECT(text_buf), "filename", g_strdup(FileToLoad), (GDestroyNotify)g_free);
	guint idle_id = g_idle_add((GSourceFunc)open_file, text_buf);
	GtkWidget *plugwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(plugwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), plugwin);
	GtkWidget *view = gtk_text_view_new_with_buffer(text_buf);
	gtk_widget_modify_font(view, pango_font_description_from_string(gFont));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
	gtk_container_add(GTK_CONTAINER(plugwin), GTK_WIDGET(view));

	if (ShowFlags & lcp_wraptext)
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);
	else
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_NONE);

	g_object_set_data_full(G_OBJECT(plugwin), "text_buf", text_buf, (GDestroyNotify)g_object_unref);
	g_object_set_data(G_OBJECT(plugwin), "id", GUINT_TO_POINTER(idle_id));
	g_object_set_data(G_OBJECT(text_buf), "text_view", view);
	gtk_widget_show_all(plugwin);

	return plugwin;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	guint idle_id = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(ListWin), "id"));

	if (idle_id > 0)
		g_source_remove(idle_id);

	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	GtkTextBuffer *text_buf;
	GtkTextIter p;

	text_buf = g_object_get_data(G_OBJECT(ListWin), "text_buf");

	switch (Command)
	{
	case lc_copy :
		gtk_text_buffer_copy_clipboard(GTK_TEXT_BUFFER(text_buf), gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
		break;

	case lc_selectall :
		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(text_buf), &p);
		gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(text_buf), &p);
		gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(text_buf), &p);
		gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(text_buf), "selection_bound", &p);
		break;

	case lc_newparams :
		if (Parameter & lcp_wraptext)
			gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_object_get_data(G_OBJECT(text_buf), "text_view")), GTK_WRAP_WORD_CHAR);
		else
			gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_object_get_data(G_OBJECT(text_buf), "text_view")), GTK_WRAP_NONE);

		break;

	default :
		return LISTPLUGIN_ERROR;
	}
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	gboolean found;
	GtkTextIter iter, mstart, mend;

	GtkTextBuffer *text_buf = g_object_get_data(G_OBJECT(ListWin), "text_buf");

	if (!text_buf)
		return LISTPLUGIN_ERROR;

	GtkTextMark *last_pos = gtk_text_buffer_get_mark(GTK_TEXT_BUFFER(text_buf), "last_pos");

	if (last_pos == NULL)
		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(text_buf), &iter);
	else
		gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(text_buf), &iter, last_pos);

	if (SearchParameter & lcs_backwards)
		found = gtk_text_iter_backward_search(&iter, SearchString, GTK_TEXT_SEARCH_TEXT_ONLY, &mend, &mstart, NULL);
	else
		found = gtk_text_iter_forward_search(&iter, SearchString, GTK_TEXT_SEARCH_TEXT_ONLY, &mstart, &mend, NULL);

	if (found)
	{
		gtk_text_buffer_select_range(GTK_TEXT_BUFFER(text_buf), &mstart, &mend);
		gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(text_buf), "last_pos", &mend, FALSE);
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(g_object_get_data(G_OBJECT(text_buf), "text_view")),
		                                   gtk_text_buffer_get_mark(GTK_TEXT_BUFFER(text_buf), "last_pos"));
	}
	else
	{
		show_error(GTK_WIDGET(ListWin), GTK_MESSAGE_INFO, _("\"%s\" not found!"), SearchString);
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	static char plg_path[PATH_MAX];
	const char* loc_dir = "langs";
	GKeyFile *cfg;
	char cfg_path[PATH_MAX];

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(plg_path, &dlinfo) != 0)
	{
		strncpy(plg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plg_path, '/');

		if (pos)
			strcpy(pos + 1, loc_dir);

		setlocale(LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, plg_path);
		textdomain(GETTEXT_PACKAGE);
	}

	g_strlcpy(cfg_path, dps->DefaultIniName, PATH_MAX);

	char *pos = strrchr(cfg_path, '/');

	if (pos)
		strcpy(pos + 1, "j2969719.ini");

	cfg = g_key_file_new();

	g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL);

	if (!g_key_file_has_key(cfg, PLUGNAME, "font", NULL))
	{
		g_key_file_set_string(cfg, PLUGNAME, "font", gFont);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
	}
	else
	{
		gchar *font = g_key_file_get_string(cfg, PLUGNAME, "font", NULL);
		g_strlcpy(gFont, font, MAX_PATH);
		g_free(font);
	}

	g_key_file_free(cfg);
}


void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen - 1);
}
