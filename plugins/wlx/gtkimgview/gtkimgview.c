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
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	view = gtk_image_view_new ();
	scroll = gtk_image_scroll_win_new(GTK_IMAGE_VIEW(view));
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	pixbuf = gdk_pixbuf_new_from_file(FileToLoad, NULL);
	if (!pixbuf)
	{
		gtk_widget_destroy(gFix);
		return NULL;
	}
	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, TRUE);
	g_object_set_data_full(G_OBJECT(gFix), "pixbuf", pixbuf, (GDestroyNotify) g_object_unref);
	gtk_widget_show_all(gFix);
	
	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString,int maxlen)
{
	strncpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSendCommand (HWND ListWin,int Command,int Parameter)
{
	GdkPixbuf *pixbuf;

	pixbuf = g_object_get_data(G_OBJECT(ListWin), "pixbuf");

	switch(Command)
	{
		case lc_copy :
			gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), pixbuf);
			break;
		default :
			return LISTPLUGIN_ERROR;
	}
}

int DCPCALL ListSearchDialog(HWND ListWin,int FindNext)
{
	return LISTPLUGIN_OK;
}
