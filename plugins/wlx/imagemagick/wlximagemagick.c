#include <gtkimageview/gtkimagescrollwin.h>
#include <gtkimageview/gtkimageview.h>
#include <wand/MagickWand.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "(EXT=\"DDS\")|(EXT=\"TGA\")|(EXT=\"PCX\")"


HWND DCPCALL ListLoad (HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *view;
	GdkPixbuf *pixbuf;
	gint width;
	gint height;
	gint depth;
	gint rowstride;
	gint row;
	guchar *pixels;
	MagickWand *magick_wand;

	gFix = gtk_vbox_new(FALSE , 1);
	gtk_container_add(GTK_CONTAINER (GTK_WIDGET(ParentWin)), gFix);
	view = gtk_image_view_new ();
	scroll = gtk_image_scroll_win_new (GTK_IMAGE_VIEW (view));
	gtk_container_add (GTK_CONTAINER(gFix), scroll);
	MagickWandGenesis();
	magick_wand=NewMagickWand();

	if (MagickReadImage(magick_wand,FileToLoad) == MagickFalse)
		return NULL;
	MagickResetIterator(magick_wand);
	width = MagickGetImageWidth (magick_wand);
	height = MagickGetImageHeight (magick_wand);
	depth = MagickGetImageDepth (magick_wand);
	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, depth, width, height);
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	for (row = 0; row < height; row++) 
	{
		guchar *data = pixels + row * rowstride;
		MagickExportImagePixels (magick_wand, 0, row, width, 1, "RGBA", CharPixel, data);
	}

	if (pixbuf) 
		gtk_image_view_set_pixbuf (GTK_IMAGE_VIEW (view), pixbuf, TRUE);

	magick_wand=DestroyMagickWand(magick_wand);
	MagickWandTerminus();
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
