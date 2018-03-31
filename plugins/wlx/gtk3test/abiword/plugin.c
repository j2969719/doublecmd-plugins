#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "(EXT=\"DOC\")|(EXT=\"RTF\")|(EXT=\"DOT\")|(EXT=\"ABW\")|(EXT=\"AWT\")|(EXT=\"ZABW\")"

static void show_nope(GtkWidget *widget)
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
	                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
	                    "nope:(");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	Dl_info dlinfo;
	GtkWidget *gFix;
	GtkWidget *socket;
	static char kpath[PATH_MAX];

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(kpath, &dlinfo) != 0)
	{
		g_strlcpy(kpath, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(kpath, '/');

		if (pos)
			strcpy(pos + 1, "kostyl");
	}

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);

	socket = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gFix), socket);
	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(socket));
	gchar *command = g_strdup_printf("\"%s\" -w %d -f \"%s\"", kpath, id, FileToLoad);
	g_print(command);

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
	g_strlcpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	show_nope(GTK_WIDGET(ListWin));
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	show_nope(GTK_WIDGET(ListWin));
	return LISTPLUGIN_OK;
}
