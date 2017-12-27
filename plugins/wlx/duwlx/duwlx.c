#include <gtk/gtk.h>
#include "wlxplugin.h"

#define DETECT_STRING "EXT=\"*\""

HANDLE DCPCALL ListLoad (HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *tView;
	GtkTextBuffer *tBuf;
	gchar *tmp, *command, *buf1;

	gchar *content_type = g_content_type_guess (FileToLoad, NULL, 0, NULL);
	if (!g_content_type_is_mime_type (content_type, "inode/directory"))
	{
		g_free(content_type);
		return NULL;
	}
	g_free(content_type);
	command = g_strdup_printf("sh -c 'cd \"%s\" && du -h --apparent-size | sort -hr'", FileToLoad);
	if (!g_spawn_command_line_sync(command, &buf1, NULL, NULL, NULL))
	{
		g_free(command);
		return NULL;
	}
	g_free(command);

	gFix = gtk_vbox_new(FALSE , 5);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tBuf = gtk_text_buffer_new(NULL);
	g_object_set_data_full(G_OBJECT(gFix), "txtbuf", tBuf, (GDestroyNotify)g_object_unref);
	tmp = g_locale_to_utf8(buf1, -1, NULL, NULL, NULL);
	g_free(buf1);
	if (tmp == NULL)
	{
		gtk_widget_destroy(GTK_WIDGET(gFix));
		return NULL;
	}
	gtk_text_buffer_set_text(tBuf, tmp, -1);

	tView = gtk_text_view_new_with_buffer(tBuf);
	gtk_widget_modify_font (tView, pango_font_description_from_string("Monospace 11"));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tView), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (tView), FALSE);

	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(tView));
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	gtk_widget_show_all(gFix);

	g_free(tmp);

	return gFix;

}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
    g_strlcpy(DetectString, DETECT_STRING, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin,int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand (HWND ListWin,int Command,int Parameter)
{
	GtkTextBuffer *tBuf;
	GtkTextIter p;

	tBuf = g_object_get_data (G_OBJECT(ListWin), "txtbuf");

	switch(Command)
	{
		case lc_copy :
			gtk_text_buffer_copy_clipboard(GTK_TEXT_BUFFER(tBuf), gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
			break;
		case lc_selectall :
			gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(tBuf), &p);
			gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(tBuf), &p);
			gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(tBuf), &p);
			gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(tBuf), "selection_bound", &p);
			break;
		default :
			return LISTPLUGIN_ERROR;
	}
}
