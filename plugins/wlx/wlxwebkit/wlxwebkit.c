#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "(EXT=\"HTML\")|(EXT=\"HTM\")|(EXT=\"XHTM\")"


GtkWidget* find_child(GtkWidget* parent, const gchar* name)
{
	if (g_ascii_strcasecmp(gtk_widget_get_name((GtkWidget*)parent), (gchar*)name) == 0) 
	{
		return parent;
	}

	if (GTK_IS_BIN(parent)) 
	{
		GtkWidget *child = gtk_bin_get_child(GTK_BIN(parent));
		return find_child(child, name);
	}

	if (GTK_IS_CONTAINER(parent)) 
	{
		GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
		while ((children = g_list_next(children)) != NULL) 
			{
				GtkWidget* widget = find_child(children->data, name);
				if (widget != NULL) 
					{
						return widget;
					}
			}
	}

	return NULL;
}


HWND DCPCALL ListLoad (HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget* webView;

	gFix = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);
	webView = webkit_web_view_new();
	gtk_widget_set_name (webView, "webkitfrm");
	gchar* fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);
	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webView), fileUri);
	gtk_widget_grab_focus(webView);
	gtk_container_add (GTK_CONTAINER (gFix), webView);

	gtk_widget_show_all (gFix); 
	return gFix;
}

void ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(ListWin);
}

void DCPCALL ListGetDetectString(char* DetectString,int maxlen)
{
	strncpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchText(HWND ListWin,char* SearchString,int SearchParameter)
{
	bool ss_case = FALSE;
	bool ss_forward = TRUE;
	if (SearchParameter & lcs_matchcase)
		ss_case = TRUE;
	if (SearchParameter & lcs_backwards)
		ss_forward = FALSE;
	webkit_web_view_search_text (WEBKIT_WEB_VIEW(find_child(ListWin, "webkitfrm")), SearchString, ss_case, ss_forward, TRUE);

}

int DCPCALL ListSendCommand(HWND ListWin,int Command,int Parameter)
{
	switch(Command) 
	{
		case lc_copy :
			webkit_web_view_copy_clipboard (WEBKIT_WEB_VIEW(find_child(ListWin, "webkitfrm")));
			break;
		case lc_selectall :
			webkit_web_view_select_all (WEBKIT_WEB_VIEW(find_child(ListWin, "webkitfrm")));
			break;
		default :
			printf("Command = %d\n", Command);
	}
}

