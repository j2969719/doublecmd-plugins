#include <gtk/gtk.h>
#include <LibreOfficeKit/LibreOfficeKitGtk.h> 
#include "wlxplugin.h"

#define TEST_PATH "/usr/lib/libreoffice/program"

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), scroll);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	GtkWidget *view = lok_doc_view_new(TEST_PATH, NULL, NULL);
	//lok_doc_view_set_edit(LOK_DOC_VIEW(view), FALSE);
	gtk_container_add(GTK_CONTAINER(scroll), view);
	lok_doc_view_open_document(LOK_DOC_VIEW(view), FileToLoad, "{}", NULL, NULL, NULL);
	g_object_set_data(G_OBJECT(scroll), "view", LOK_DOC_VIEW(view));
	gtk_widget_show_all(scroll);

	return scroll;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	LOKDocView *view = (LOKDocView*)g_object_get_data(G_OBJECT(PluginWin), "view");
	lok_doc_view_open_document(LOK_DOC_VIEW(view), FileToLoad, "{}", NULL, NULL, NULL);
	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	LOKDocView *view = (LOKDocView*)g_object_get_data(G_OBJECT(ListWin), "view");

	if (SearchParameter & lcs_backwards)
		lok_doc_view_find_prev(view, SearchString, FALSE);
	else
		lok_doc_view_find_next(view, SearchString, FALSE);

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	LOKDocView *view = (LOKDocView*)g_object_get_data(G_OBJECT(ListWin), "view");

	switch (Command)
	{
	case lc_copy :
		//lok_doc_view_copy_selection(view, NULL, NULL);
		lok_doc_view_post_command(view, ".uno:Copy", NULL, FALSE);
		break;

	case lc_selectall :
		lok_doc_view_post_command(view, ".uno:SelectAll", NULL, FALSE);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen);
}
