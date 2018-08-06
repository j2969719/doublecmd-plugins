#include <gtk/gtk.h>
#include <atril-view.h>
#include <atril-document.h>
#include "wlxplugin.h"

#define _detectstring "(EXT=\"PDF\")|(EXT=\"DJVU\")|(EXT=\"DJV\")|(EXT=\"TIF\")|\
(EXT=\"TIFF\")|(EXT=\"PS\")|(EXT=\"CBR\")|(EXT=\"CBZ\")|(EXT=\"XPS\")"


static GtkWidget * getFirstChild(GtkWidget *w)
{
	GList *list = gtk_container_get_children(GTK_CONTAINER(w));
	GtkWidget *result = GTK_WIDGET(list->data);
	g_list_free(list);
	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget *gFix;
	GtkWidget *scrolled_window;
	GtkWidget *view;
	EvDocument *document;
	EvDocumentModel *docmodel;

	if (!ev_init())
		return NULL;

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gchar* fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);
	document = EV_DOCUMENT(ev_document_factory_get_document(fileUri, NULL));

	g_free(fileUri);

	if (EV_IS_DOCUMENT(document))
	{
		docmodel = EV_DOCUMENT_MODEL(ev_document_model_new_with_document(document));
		view = ev_view_new();
		ev_view_set_model(EV_VIEW(view), docmodel);
		g_object_unref(document);
		g_object_unref(docmodel);
		gtk_container_add(GTK_CONTAINER(scrolled_window), view);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		                               GTK_POLICY_AUTOMATIC,
		                               GTK_POLICY_AUTOMATIC);
	}
	else
	{
		gtk_widget_destroy(GTK_WIDGET(gFix));
		return NULL;
	}

	gtk_container_add(GTK_CONTAINER(gFix), scrolled_window);
	gtk_widget_show_all(gFix);

	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen-1);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	switch (Command)
	{
	case lc_copy :
		ev_view_copy(EV_VIEW(getFirstChild(getFirstChild(GTK_WIDGET(ListWin)))));
		break;

	case lc_selectall :
		ev_view_select_all(EV_VIEW(getFirstChild(getFirstChild(GTK_WIDGET(ListWin)))));
		break;

	default :
		return LISTPLUGIN_ERROR;
	}
}
