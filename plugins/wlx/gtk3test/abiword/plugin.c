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
	                    "not implemented:(");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void plug_added(GtkWidget *widget, gpointer data)
{
	gtk_spinner_stop(GTK_SPINNER(data));
	gtk_widget_hide(GTK_WIDGET(data));
	gtk_widget_show(widget);
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	Dl_info dlinfo;
	GtkWidget *gFix;
	GtkWidget *wspin;
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
	wspin = gtk_spinner_new();
	gtk_spinner_start(GTK_SPINNER(wspin));
	gtk_box_pack_start(GTK_BOX(gFix), wspin, TRUE, FALSE, 5);
	gtk_widget_show(wspin);

	socket = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gFix), socket);
	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(socket));
	gchar *command = g_strdup_printf("\"%s\" -w %d -f \"%s\"", kpath, id, FileToLoad);
	//g_print("%s\n", command);

	if (!g_spawn_command_line_async(command, NULL))
	{
		g_free(command);
		gtk_widget_destroy(gFix);
		return NULL;
	}

	g_free(command);
	g_signal_connect(G_OBJECT(socket), "plug-added", G_CALLBACK(plug_added), (gpointer)wspin);

	gtk_widget_show(gFix);

	return gFix;

}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen-1);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	show_nope(GTK_WIDGET(ListWin));
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	if (Command == lc_copy)
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
		                       gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY)), -1);
	else
		show_nope(GTK_WIDGET(ListWin));

	return LISTPLUGIN_OK;
}
