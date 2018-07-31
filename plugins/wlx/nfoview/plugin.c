#include <gtk/gtk.h>
#include "wlxplugin.h"


HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	gchar *contents, *result;
	if (!g_file_get_contents(FileToLoad, &contents, NULL, NULL))
		return NULL;

	result = g_convert(contents, -1, "UTF-8", "866", NULL, NULL, NULL);

	if ((result == NULL) || (g_strcmp0(result, "") == 0) || (!g_utf8_validate(result, -1, NULL)))
		return NULL;

	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *tView;
	GtkTextBuffer *tBuf;
	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tBuf = gtk_text_buffer_new(NULL);
	g_object_set_data_full(G_OBJECT(gFix), "txtbuf", tBuf, (GDestroyNotify)g_object_unref);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(tBuf), result, -1);

	tView = gtk_text_view_new_with_buffer(tBuf);
	gtk_widget_modify_font(tView, pango_font_description_from_string("xos4 Terminess Powerline Bold 12"));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tView), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tView), FALSE);
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(tView));
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	gtk_widget_show_all(gFix);
	return gFix;

}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, "EXT=\"NFO\"", maxlen-1);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	GtkTextBuffer *tBuf;
	GtkTextIter p;

	tBuf = g_object_get_data(G_OBJECT(ListWin), "txtbuf");

	switch (Command)
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
