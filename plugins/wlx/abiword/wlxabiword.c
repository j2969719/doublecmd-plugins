#include <gtk/gtk.h>
#include <string.h>
#include <libabiword.h>
#include <abiwidget.h>
#include "wlxplugin.h"

GtkWidget * abi;


HWND DCPCALL ListLoad (HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget *gFix, window;
	gFix = gtk_vbox_new(FALSE , 5);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);
	libabiword_init(NULL, NULL);
	abi = abi_widget_new ();
	gtk_container_add (GTK_CONTAINER (gFix), GTK_WIDGET(abi));
	gchar* fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);
	abi_widget_load_file (ABI_WIDGET (abi), fileUri, "");
        abi_widget_view_print_layout (abi);
	gtk_widget_show_all (gFix);
	return gFix;
	libabiword_shutdown ();
}

void ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(ListWin);
}

void DCPCALL ListGetDetectString(char* DetectString,int maxlen)
{
	strncpy(DetectString, "(EXT=\"DOC\")|(EXT=\"RTF\")|(EXT=\"DOT\")|(EXT=\"ABW\")|(EXT=\"AWT\")|(EXT=\"ZABW\")", maxlen);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString,int SearchParameter)
{
        abi_widget_set_find_string (abi, SearchString);
	abi_widget_find_prev (abi);
}

int DCPCALL ListSendCommand(HWND ListWin,int Command,int Parameter)
{
	switch(Command)
	{
		case lc_copy :
			abi_widget_copy (abi);
			break;
		case lc_selectall :
			abi_widget_select_all (abi);
			break;
		default :
			printf("Command = %d\n", Command);
	}
}