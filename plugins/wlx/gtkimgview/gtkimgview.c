#include <gtkimageview/gtkimagescrollwin.h>
#include <gtkimageview/gtkimageview.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "(EXT=\"JPG\")|(EXT=\"PNG\")|(EXT=\"SVG\")|(EXT=\"BMP\")|(EXT=\"ICO\")"

HWND DCPCALL ListLoad (HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *view;
	GdkPixbuf *pixbuf;
	
	gFix = gtk_vbox_new(FALSE , 1);
	gtk_container_add(GTK_CONTAINER (GTK_WIDGET(ParentWin)), gFix);
	view = gtk_image_view_new ();
	scroll = gtk_image_scroll_win_new (GTK_IMAGE_VIEW (view));
	gtk_container_add (GTK_CONTAINER(gFix), scroll);
	pixbuf = gdk_pixbuf_new_from_file (FileToLoad, NULL);
	if (!pixbuf)
		return NULL;
	gtk_image_view_set_pixbuf (GTK_IMAGE_VIEW (view), pixbuf, TRUE);
	gtk_widget_show_all (gFix);
	
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
