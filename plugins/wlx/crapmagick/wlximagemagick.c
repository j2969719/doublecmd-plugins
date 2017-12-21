#include <gtkimageview/gtkimagescrollwin.h>
#include <gtkimageview/gtkimageview.h>
#include <MagickWand/MagickWand.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "(EXT=\"TTF\")|(EXT=\"OTF\")|(EXT=\"PCX\")"

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *view;
	GtkWidget *scroll;
	GdkPixbuf *pixbuf;
	MagickBooleanType status;
	MagickWand *magick_wand;

	gFix = gtk_vbox_new(FALSE , 1);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	view = gtk_image_view_new ();
	scroll = gtk_image_scroll_win_new(GTK_IMAGE_VIEW(view));
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	MagickWandGenesis();
	magick_wand=NewMagickWand();
	status=MagickReadImage(magick_wand,FileToLoad);
	if (status != MagickFalse)
	{
		MagickResetIterator(magick_wand);
		status=MagickWriteImages(magick_wand, "/tmp/_dcwlximagemagick.png", MagickTrue);
	}
	magick_wand=DestroyMagickWand(magick_wand);
	MagickWandTerminus();
	if (status != MagickFalse)
	{
		pixbuf = gdk_pixbuf_new_from_file("/tmp/_dcwlximagemagick.png", NULL);
		if (!pixbuf)
		{
			gtk_widget_destroy(gFix);
			return NULL;
		}
		gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, TRUE);
		g_object_set_data_full(G_OBJECT(gFix), "pixbuf", pixbuf, (GDestroyNotify) g_object_unref);
	}
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
