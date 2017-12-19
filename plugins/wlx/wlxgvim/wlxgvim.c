#include <gtk/gtk.h>
#include <string.h>
#include "gtkvim.h"
#include "wlxplugin.h"

#define _detectstring  "(EXT=\"C\")|(EXT=\"H\")"

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget *gFix;
	GtkWidget *vim;

	gFix = gtk_vbox_new(FALSE , 5);
	gtk_container_add(GTK_CONTAINER (GTK_WIDGET(ParentWin)), gFix);

	vim = gtk_vim_new(40, 40, FileToLoad);
	gtk_container_add(GTK_CONTAINER(gFix), vim);
	gtk_widget_realize(vim);

	gtk_widget_show_all(gFix);

	return gFix;
}

void ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString,int maxlen)
{
	strncpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin,int FindNext)
{
	return LISTPLUGIN_OK;
}
