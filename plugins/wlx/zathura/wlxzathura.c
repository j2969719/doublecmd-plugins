#include <gtk/gtk.h>
#include "wlxplugin.h"

#define _detectstring  "(EXT=\"PDF\")|(EXT=\"DJVU\")|(EXT=\"DJV\")|(EXT=\"PS\")|(EXT=\"CBR\")|(EXT=\"CBZ\")"


HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget *gFix;
	GtkWidget *zathura;

	gFix = gtk_vbox_new(FALSE , 5);
	gtk_container_add(GTK_CONTAINER (GTK_WIDGET(ParentWin)), gFix);

	zathura = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gFix), zathura);
	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(zathura));
	gchar *command = g_strdup_printf("zathura --reparent=%d \"%s\"", id, FileToLoad);
	g_spawn_command_line_async(command, NULL);
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
	return LISTPLUGIN_OK;
}
