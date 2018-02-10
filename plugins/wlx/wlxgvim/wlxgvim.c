#include <gtk/gtk.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring  "(EXT=\"C\")|(EXT=\"H\")"
#define _cmdstring "gvim --servername %d --socketid %d \"%s\" -R"

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget *gFix;
	GtkWidget *vim;

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);

	vim = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gFix), vim);
	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(vim));
	gchar *command = g_strdup_printf(_cmdstring, id, id, FileToLoad);

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

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	strncpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}
