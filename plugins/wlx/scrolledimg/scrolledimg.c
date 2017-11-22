#include <gtk/gtk.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "EXT=\"GIF\""

HWND DCPCALL ListLoad (HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget *gFix;
	GtkWidget *scrolled_window;
	GtkWidget *image;

	gFix = gtk_vbox_new(FALSE , 5);
	gtk_container_add(GTK_CONTAINER (GTK_WIDGET(ParentWin)), gFix);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 1);
	image = gtk_image_new_from_file (FileToLoad);

	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), image);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (gFix), scrolled_window);

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
