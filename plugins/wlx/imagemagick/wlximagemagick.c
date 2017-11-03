#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <wand/MagickWand.h>
#include "wlxplugin.h"

HWND DCPCALL ListLoad (HWND ParentWin, char* FileToLoad, int ShowFlags)
{
  GtkWidget *gFix;
  GtkWidget *image;
  GtkWidget *scroll;
  GdkPixbuf *pixbuf;
  gint width;
  gint height;
  gint rowstride;
  gint row;
  guchar *pixels;
  MagickBooleanType status;
  MagickWand *magick_wand;

  gFix = gtk_vbox_new(FALSE , 1);
  gtk_container_add(GTK_CONTAINER (GTK_WIDGET(ParentWin)), gFix);
  gtk_widget_show (gFix);
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER(gFix), scroll);
  gtk_widget_show (scroll);
  MagickWandGenesis();
  magick_wand=NewMagickWand();
  MagickReadImage(magick_wand,FileToLoad);
  MagickResetIterator(magick_wand);
  width = MagickGetImageWidth (magick_wand);
  height = MagickGetImageHeight (magick_wand);
  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  pixels = gdk_pixbuf_get_pixels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  MagickSetImageDepth (magick_wand, 8);

  for (row = 0; row < height; row++) {
      guchar *data = pixels + row * rowstride;
      MagickExportImagePixels (magick_wand, 0, row, width, 1, "RGBA", CharPixel, data);
  }

  if (pixbuf) {
    image = gtk_image_new_from_pixbuf (pixbuf);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), image);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show (image);

  }
  magick_wand=DestroyMagickWand(magick_wand);
  MagickWandTerminus();
  return gFix;
}

void ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString,int maxlen)
{
	strncpy(DetectString, "(EXT=\"DDS\")|(EXT=\"TGA\")|(EXT=\"PCX\")", maxlen);
}