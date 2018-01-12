#include <gtk/gtk.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "(EXT=\"HTML\")|(EXT=\"HTM\")|(EXT=\"XHTM\")|(EXT=\"XHTML\")"
#define _cmdstring "vimb -e %d \"%s\""

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget *gFix;
	GtkWidget *socket;

	gFix = gtk_vbox_new(FALSE , 5);
	gtk_container_add(GTK_CONTAINER (GTK_WIDGET(ParentWin)), gFix);

	socket = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gFix), socket);
	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(socket));
	gchar *command = g_strdup_printf(_cmdstring, id, FileToLoad);
	if (!g_spawn_command_line_async(command, NULL))
	{
		g_free(command);
		gtk_widget_destroy(gFix);
		return NULL;
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
	strncpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin,int FindNext)
{
	return LISTPLUGIN_OK;
}
