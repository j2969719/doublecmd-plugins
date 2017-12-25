#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "wlxplugin.h"

#define _detectstring  "(EXT=\"AVI\")|(EXT=\"MKV\")|(EXT=\"FLV\")|(EXT=\"MPG\")|\
(EXT=\"MPEG\")|(EXT=\"MP4\")|(EXT=\"3GP\")|(EXT=\"MP3\")|(EXT=\"OGG\")|(EXT=\"WMA\")|\
(EXT=\"BIK\")|(EXT=\"VOC\")|(EXT=\"WAV\")|(EXT=\"WEBM\")|(EXT=\"VOB\")|(EXT=\"ROQ\")|\
(EXT=\"IVF\")|(EXT=\"MOV\")"


HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget *gFix;
	GtkWidget *mpv;
	GtkWidget *label;

	gFix = gtk_vbox_new(FALSE , 5);
	gtk_container_add(GTK_CONTAINER (GTK_WIDGET(ParentWin)), gFix);

	mpv = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(gFix), mpv);
	GdkNativeWindow id = GDK_WINDOW_XID(gtk_widget_get_window(mpv));
	gchar *command = g_strdup_printf("mpv --force-window=yes --loop --wid=%d \"%s\"", id, FileToLoad);
	if (id!=0)
		g_spawn_command_line_async(command, NULL);
	else
	{
		gtk_widget_destroy(mpv);
		label = gtk_label_new("no luck:(\nmove the cursor up or down...");
		gtk_container_add(GTK_CONTAINER(gFix), label);
	}
	g_free(command);

	gtk_widget_show_all(gFix);

	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString,int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin,int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin,int Command,int Parameter)
{
	return LISTPLUGIN_ERROR;
}
