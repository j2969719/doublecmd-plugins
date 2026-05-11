#include <gtk/gtk.h>
#include <atril-view.h>
#include <atril-document.h>
#include "wlxplugin.h"

gboolean gEvInit = FALSE;

static EvDocument* get_ev_document(char* FileToLoad)
{
	gchar *uri = g_filename_to_uri(FileToLoad, NULL, NULL);

	if (!uri)
		return NULL;

	GError *err = NULL;
	EvDocument *document = EV_DOCUMENT(ev_document_factory_get_document(uri, &err));
	g_free(uri);

	if (err || !EV_IS_DOCUMENT(document))
	{
		if (err)
		{
			g_print("%s (%s): %s\n", PLUGNAME, FileToLoad, err->message);
			g_error_free(err);
		}

		if (document)
			g_object_unref(document);

		return NULL;
	}

	return document;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	if (!gEvInit)
		return NULL;

	EvDocument *document = get_ev_document(FileToLoad);

	if (!document)
		return NULL;

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), scroll);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);

	GtkWidget *view = ev_view_new();
	gtk_container_add(GTK_CONTAINER(scroll), view);

	EvDocumentModel *docmodel = EV_DOCUMENT_MODEL(ev_document_model_new_with_document(document));
	ev_view_set_model(EV_VIEW(view), docmodel);
	ev_view_find_set_highlight_search(EV_VIEW(view), TRUE);
	g_object_unref(document);
	g_object_unref(docmodel);

	g_object_set_data(G_OBJECT(scroll), "view", EV_VIEW(view));
	g_object_set_data(G_OBJECT(scroll), "document", document);
	g_object_set_data(G_OBJECT(scroll), "findfirst", GINT_TO_POINTER(TRUE));

	gtk_widget_show_all(scroll);

	return scroll;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	EvDocument *document = get_ev_document(FileToLoad);

	if (!document)
		return LISTPLUGIN_ERROR;

	EvView *view = (EvView*)g_object_get_data(G_OBJECT(PluginWin), "view");
	ev_view_find_cancel(view);
	EvDocument *olddocument = (EvDocument*)g_object_get_data(G_OBJECT(PluginWin), "document");
	g_object_unref(olddocument);
	EvDocumentModel *docmodel = EV_DOCUMENT_MODEL(ev_document_model_new_with_document(document));
	ev_view_set_model(view, docmodel);
	g_object_set_data(G_OBJECT(PluginWin), "document", document);
	g_object_set_data(G_OBJECT(PluginWin), "findfirst", GINT_TO_POINTER(TRUE));
	g_object_unref(docmodel);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	EvDocument *document = (EvDocument*)g_object_get_data(G_OBJECT(ListWin), "document");
	g_object_unref(document);

	gtk_widget_destroy(GTK_WIDGET(ListWin));
}
/*
int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	EvView *view = (EvView*)g_object_get_data(G_OBJECT(ListWin), "view");
	gboolean findfirst = (gboolean)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(ListWin), "findfirst"));

	if (SearchParameter & lcs_findfirst || findfirst)
	{
		ev_view_find_cancel(view);
		g_object_set_data(G_OBJECT(ListWin), "findfirst", GINT_TO_POINTER(FALSE));
		EvDocument *document = (EvDocument*)g_object_get_data(G_OBJECT(ListWin), "document");
		EvJob *job = ev_job_find_new(document,
		                             0,
		                             ev_document_get_n_pages(document),
		                             SearchString,
		                             (SearchParameter & lcs_matchcase));
		ev_view_find_started(view, EV_JOB_FIND(job));
		ev_job_scheduler_push_job(job, EV_JOB_PRIORITY_LOW);
		g_object_unref(job);
	}
	else
	{
		if (SearchParameter & lcs_backwards)
			ev_view_find_previous(view);
		else
			ev_view_find_next(view);
	}


	return LISTPLUGIN_OK;
}
*/
int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	EvView *view = (EvView*)g_object_get_data(G_OBJECT(ListWin), "view");
	ev_view_find_cancel(view);

	switch (Command)
	{
	case lc_copy :
		ev_view_copy(view);
		break;

	case lc_selectall :
		ev_view_select_all(view);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

static void wlxplug_atexit(void)
{
	printf("%s atexit\n", PLUGNAME);
	ev_shutdown();
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	if (!gEvInit)
	{
		gEvInit = ev_init();
		atexit(wlxplug_atexit);
	}
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen);
}
